using System;
using System.IO;
using System.Text;

using XmlFmt;
using SharpFuzz;

public class Program {
    // public static bool EqualFiles(string lhs, string rhs) {
    //     byte[] lhsBytes = File.ReadAllBytes(lhs);
    //     byte[] rhsBytes = File.ReadAllBytes(rhs);
    //     if (lhsBytes.Length != rhsBytes.Length) {
    //         return false;
    //     }
    //     for (int i = 0; i < lhsBytes.Length; ++i) {
    //         if (lhsBytes[i] != rhsBytes[i]) {
    //             return false;
    //         }
    //     }
    //     return true;
    // }

    public static void WriteBinaryFromString(string stream, string filename) {
        BinaryWriter writer = new BinaryWriter(File.Open(filename, FileMode.Create));
        writer.Write(Encoding.ASCII.GetBytes(stream));
        writer.Close();
    }

    public static void Main(string[] args) {
        Fuzzer.OutOfProcess.Run(stream => {
            try {
                WriteBinaryFromString(stream, "test.bin");
                XmlFmtProgram.BinToXml("test.bin", "intra.xml");
                XmlFmtProgram.XmlToBin("intra.xml", "result.bin");
                if (!XmlFmt.FilesComporator.CompareBinFiles("test.bin", "result.bin")) {
                    throw new ArgumentOutOfRangeException("Files are not equal!");
                }
            } catch (XmlFmt.ParseException) {
                // handle expected exception
                return;
            }
        });
    }
}
