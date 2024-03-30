using System;
using System.IO;
using System.Text;

using XmlFmt;
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
