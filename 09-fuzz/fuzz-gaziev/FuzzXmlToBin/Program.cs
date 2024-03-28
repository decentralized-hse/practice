using System;
using System.IO;
using System.Text;

using XmlFmt;
using SharpFuzz;

public class Program {
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
