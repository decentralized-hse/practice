using SharpFuzz;
using ParseLibrary;
using NLog;


public class Program
{
    private static Logger logger = LogManager.GetCurrentClassLogger();

    public static void Main(string[] args)
    {
        Fuzzer.OutOfProcess.Run(stream =>
        {
            bool fuzz_bin = true; // set to true if you want to fuzz .bin
            // if you want to fuzz .kv, set it to false
            try
            {
                if (fuzz_bin) {
                    // фаззим для .bin
                    using var reader = new BinaryReader(stream);
                    int res = MainParser.ProcessBinaryStream(reader, "test_input.bin", true);
                    if (res == -1) {
                        throw new FormatException("Exit code is -1: malformed input");
                    } else if (res != 0) {
                        throw new ArgumentOutOfRangeException("Strange after ProcessBinaryStream!");
                    }

                    res = MainParser.ProcessKV("test_input.kv");
                    if (res == -1) {
                        throw new FormatException("Exit code is -1: malformed input");
                    } else if (res != 0) {
                        throw new ArgumentOutOfRangeException("Strange after ProcessKV!");
                    }

                    bool filesEqual = MainParser.CompareFiles("test_input.bin", "test_initial.bin");
                    if (!filesEqual) {
                        Console.WriteLine("NOT EQUAL FILES");
                        throw new ArgumentOutOfRangeException("Files do not coincide!");
                    } else {
                        Console.WriteLine("Files are equal");
                    }
                } else {
                    // фаззим для .kv
                    using var reader = new StreamReader(stream);
                    
                    int res = MainParser.ProcessKVStream(reader, "test_input.kv", true);
                    if (res == -1) {
                        throw new FormatException("Exit code is -1: malformed input");
                    } else if (res != 0) {
                        throw new ArgumentOutOfRangeException("Strange after ProcessKVStream!");
                    }

                    res = MainParser.ProcessBinary("test_input.bin");
                    if (res == -1) {
                        throw new FormatException("Exit code is -1: malformed input");
                    } else if (res != 0) {
                        throw new ArgumentOutOfRangeException("Strange after ProcessBinary!");
                    }

                    bool filesEqual = MainParser.CompareFiles("test_input.kv", "test_initial.kv");
                    if (!filesEqual) {
                        Console.WriteLine("NOT EQUAL FILES");
                        throw new ArgumentOutOfRangeException("Files do not coincide!");
                    } else {
                        Console.WriteLine("Files are equal");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("EXCEPTION!!!");
                Console.WriteLine(ex.Message);

                if (ex.Message == "Exit code is -1: malformed input") {
                    Console.WriteLine("Exit code is -1: malformed input");
                    return;
                } else {
                    Console.WriteLine("????????????? ex.Message != CheckFormat failed for student");
                    throw ex; // uncomment me!!!!
                }
            }
        });
    }
}
