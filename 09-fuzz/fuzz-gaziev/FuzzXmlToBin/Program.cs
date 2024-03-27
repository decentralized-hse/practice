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

    public static void WriteStreamFromString(string stream, string filename) {
        StreamWriter writer = new StreamWriter(File.Open(filename, FileMode.Create));
        writer.Write(stream);
        writer.Close();
    }

    public static void Main(string[] args) {
        Fuzzer.OutOfProcess.Run(stream => {
            try {
                WriteStreamFromString(stream, "test.xml");
                XmlFmtProgram.XmlToBin("test.xml", "intra.bin");
                XmlFmtProgram.BinToXml("intra.bin", "result.xml");
                if (!XmlFmt.FilesComporator.CompareXmlFiles("test.xml", "result.xml")) {
                    throw new ArgumentOutOfRangeException("Files are not equal!");
                }
            } catch (XmlFmt.ParseException) {
                // handle expected exception
                return;
            }
        });
    }
}
