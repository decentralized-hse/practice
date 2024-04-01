using System;
using System.IO;
using System.Text;

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
                WriteStreamFromString(stream, "test.json");
                JsonParseProgram.JsonToBin("test.json", "test.bin");
                JsonParseProgram.BinToJson("test.bin", "result.json");
                if (!FilesComporator.CompareJsonFiles("test.json", "result.json")) {
                    throw new ArgumentOutOfRangeException("Files are not equal!");
                }
            } catch (ParseException) {
                // handle expected exception
                return;
            }
        });
    }
}
