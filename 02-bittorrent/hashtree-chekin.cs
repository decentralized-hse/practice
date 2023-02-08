using System.Security.Cryptography;
using System.Text;
using System;
using System.IO;
using System.Linq;

using HashTree;

class Program
{
    static int Main(string[] args)
    {
        const int BLOCK_SIZE = 1024;

        var reader = new ChunkReader(BLOCK_SIZE);
        if (args.Length != 1)
        {
            Console.WriteLine("Provide absolute filepath as cl arg");
            return -1;
        }

        string filePath = args[0];
        var initChunks = reader.ReadChunksFromFile(filePath);
        var treeBuilder = new MerkleTree(initChunks.Length);

        treeBuilder.Build(initChunks);
        treeBuilder.WriteTreeToFile(filePath+".hashtree.chekin");
        return 0;
    }
}

namespace HashTree
{
    public struct Chunk
    {
        public byte[] Contents;

        public Chunk(byte[] contents)
        {
            Contents = contents;
        }
    }
    
    public class ChunkReader
    {
        private int _chunkSz;

        public ChunkReader(int chunkSz)
        {
            _chunkSz = chunkSz;
        }
        public Chunk[] ReadChunksFromFile(string filePath)
        {
            Console.WriteLine($"Reading chunks from: {filePath}");
            
            Chunk[] chunks;
            var fileContents = File.ReadAllBytes(filePath);
            if (fileContents.Length % _chunkSz != 0)
            {
                throw new Exception($"File size {fileContents.Length} is not dividable by chunk size {_chunkSz}");
            }

            var chunksCnt = fileContents.Length / _chunkSz;
            chunks = new Chunk[chunksCnt];

            for (int i = 0; i < chunksCnt; ++i)
            {
                chunks[i] = new Chunk(new ArraySegment<byte>(fileContents, i * _chunkSz, _chunkSz).ToArray());
            }
            
            return chunks;
        }
    }
    
    public class MerkleTree
    {
        private static SHA256 _hasher = SHA256.Create();
        private int _treeSz;
        
        byte[] EMPTY_BLOCK = Encoding.ASCII.GetBytes(String.Concat(Enumerable.Repeat("0", 64).ToArray()) + "\n");

        private byte[][] _chunkHashes;
        
        public MerkleTree(int firstLevelSz)
        {
            _treeSz = firstLevelSz * 2 - firstLevelSz % 2;
            _chunkHashes = new byte[_treeSz][];
            for (var i = 0; i < _chunkHashes.Length; ++i)
            {
                _chunkHashes[i] = EMPTY_BLOCK;
            }
        }
        
        private byte[] ToHexString(byte[] bytes)
        {
            string hex = BitConverter.ToString(bytes).Replace("-", "").ToLower() + "\n";
            if (bytes == EMPTY_BLOCK)
            {
                hex = Encoding.ASCII.GetString(bytes) + "\n";
            }

            return Encoding.ASCII.GetBytes(hex);
        }
        private byte[] HashChunk(byte[] input)
        {
            byte[] hash = _hasher.ComputeHash(input);
            return ToHexString(hash);
        }

        private byte[] ConcatenateChunks(int leftIdx, int rightIdx)
        {
            byte[] concatenatedChunks = new byte[_chunkHashes[leftIdx].Length + _chunkHashes[rightIdx].Length];
            Buffer.BlockCopy(_chunkHashes[leftIdx], 0, concatenatedChunks, 0, _chunkHashes[leftIdx].Length);
            Buffer.BlockCopy(_chunkHashes[rightIdx], 0, concatenatedChunks, _chunkHashes[leftIdx].Length, _chunkHashes[rightIdx].Length);
            return concatenatedChunks;
        }

        private void FillFirstLevel(Chunk[] initialChunks)
        {
            for (var i = 0; i < _treeSz; i += 2)
            {
                _chunkHashes[i] = HashChunk(initialChunks[i / 2].Contents);
            }
        }

        private void FillHigherLevels()
        {
            for (var curBinPow = 2; curBinPow < _treeSz; curBinPow *= 2)
            {
                var levelStartingIdx = curBinPow / 2 - 1;
                for (var i = levelStartingIdx; i + curBinPow < _treeSz; i += curBinPow * 2)
                {
                    var leftChildIdx = i;
                    var rightChildIdx = leftChildIdx + curBinPow;
                    var curNodeIdx = (leftChildIdx + rightChildIdx) / 2;

                    if (_chunkHashes[leftChildIdx] == EMPTY_BLOCK || _chunkHashes[rightChildIdx] == EMPTY_BLOCK)
                    {
                        _chunkHashes[curNodeIdx] = EMPTY_BLOCK;
                        continue;
                    }
                    
                    _chunkHashes[curNodeIdx] = HashChunk(ConcatenateChunks(leftChildIdx, rightChildIdx));
                }
            }
        }
        
        private void FillLevels(Chunk[] chunks)
        {
            FillFirstLevel(chunks);
            FillHigherLevels();
        }

        public void Build(Chunk[] initialChunks)
        {
            FillLevels(initialChunks);
        }

        public void WriteTreeToFile(string filePath)
        {
            using (FileStream fs = File.Open(filePath, FileMode.Create))
            {
                foreach (var chunkHash in _chunkHashes)
                {
                    fs.Write(chunkHash);
                }
            } 
        }
    }
}