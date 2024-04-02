using System;
using System.IO;
using System.Text;

using SharpFuzz;

public class Program {
    public static void WriteBinaryFromString(string stream, string filename) {
        BinaryWriter writer = new BinaryWriter(File.Open(filename, FileMode.Create));
        writer.Write(Encoding.ASCII.GetBytes(stream));
        writer.Close();
    }

    public static void Main(string[] args) {
        Fuzzer.OutOfProcess.Run(stream => {
            try {
                WriteBinaryFromString(stream, "test.bin");
                JsonParseProgram.BinToJson("test.bin", "test.json");
                JsonParseProgram.JsonToBin("test.json", "result.bin");
                if (!FilesComporator.CompareBinFiles("test.bin", "result.bin")) {
                    throw new ArgumentOutOfRangeException("Files are not equal!");
                }
            } catch (ParseException) {
                // handle expected exception
                return;
            }
        });
    }
}
