using System;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;

public class Student
{
    public string name = "";
    public string login = "";
    public string group = "";
    public bool[] practice = new bool[8];
    public class Project {
        public string repo = "";
        public byte mark = 0;
    }
    public Project project = new Project();
    public float mark = 0;
}


class Program {
    static byte[] StrToPaddedBytes(string str, uint size) {
        byte[] res = new byte[size];
        byte[] strBytes = System.Text.Encoding.UTF8.GetBytes(str);
        for (uint i = 0; i < strBytes.Length & i < size; ++i) {
            res[i] = strBytes[i];
        }
        for (uint i = (uint)strBytes.Length; i < size; ++i) {
            res[i] = 0;
        }
        return res;
    }


    static void BinToXml(string inFilename) {
        Console.WriteLine($"Reading binary student data from {inFilename}");
        string outFilename = inFilename.Remove(inFilename.Length - 4, 4) + ".xml";
        long numStudents = new FileInfo(inFilename).Length / 128;
        Student[] studentArr = new Student[numStudents];

        using (var stream = File.Open(inFilename, FileMode.Open)) {
            using (var reader = new BinaryReader(stream, Encoding.UTF8, false)) {
                for (uint i = 0; i < numStudents; ++i) {
                    studentArr[i] = new Student();
                    studentArr[i].name  = Encoding.UTF8.GetString(reader.ReadBytes(32)).TrimEnd('\0');
                    studentArr[i].login = Encoding.UTF8.GetString(reader.ReadBytes(16)).TrimEnd('\0');
                    studentArr[i].group = Encoding.UTF8.GetString(reader.ReadBytes(8)).TrimEnd('\0');
                    byte[] practice = reader.ReadBytes(8);
                    for (int j = 0; j < 8; ++j) {
                        studentArr[i].practice[j] = Convert.ToBoolean(practice[j]);
                    }
                    studentArr[i].project.repo = Encoding.UTF8.GetString(reader.ReadBytes(59)).TrimEnd('\0');
                    studentArr[i].project.mark = reader.ReadByte();
                    studentArr[i].mark = reader.ReadSingle();
                }
            }
        }
        Console.WriteLine($"{numStudents} students read...");
        XmlSerializer ser = new XmlSerializer(typeof(Student[]));
        XmlSerializerNamespaces ns = new XmlSerializerNamespaces();
        ns.Add("","");
        using (var writer = new StreamWriter(outFilename)) {
            ser.Serialize(writer, studentArr, ns);
        }
        Console.WriteLine($"written to {outFilename}");
    }

    static void XmlToBin(string inFilename) {
        Console.WriteLine($"Reading xml student data from {inFilename}");
        string outFilename = inFilename.Remove(inFilename.Length - 4, 4) + ".bin";

        XmlSerializer ser = new XmlSerializer(typeof(Student[]));
        // XmlSerializerNamespaces ns = new XmlSerializerNamespaces();
        // ns.Add("","");
        Student[] studentArr;

        using (Stream reader = new FileStream(inFilename, FileMode.Open))
        {
            var deserialized = ser.Deserialize(reader);
            if (deserialized == null) {
                studentArr = new Student[0];
            } else {
                studentArr = (Student[]) deserialized;
            }
        }
        Console.WriteLine($"{studentArr.Length} students read...");

        using (var stream = File.Open(outFilename, FileMode.Create))
        {
            using (var writer = new BinaryWriter(stream, Encoding.UTF8, false))
            {
                for (uint i = 0; i < studentArr.Length; ++i) {
                    byte[] practice = new byte[8];
                    for (int j = 0; j < 8; ++j) {
                        practice[j] = Convert.ToByte(studentArr[i].practice[j]);
                    }
                    writer.Write(StrToPaddedBytes(studentArr[i].name , 32));
                    writer.Write(StrToPaddedBytes(studentArr[i].login, 16));
                    writer.Write(StrToPaddedBytes(studentArr[i].group, 8));
                    writer.Write(practice);
                    writer.Write(StrToPaddedBytes(studentArr[i].project.repo, 59));
                    writer.Write(studentArr[i].project.mark);
                    writer.Write(studentArr[i].mark);
                }
            }
        }

        Console.WriteLine($"written to {outFilename}");
    }

    static int Main(string[] args) {
        if (args.Length != 1) {
            Console.WriteLine("Provide a single argument - student data file for transformation");
            return 1;
        }
        if (!File.Exists(args[0])) {
            Console.WriteLine("File does not exist");
            return 1;
        }
        if (args[0].EndsWith(".bin")) {
            BinToXml(args[0]);
            return 0;
        }
        if (args[0].EndsWith(".xml")) {
            XmlToBin(args[0]);
            return 0;
        }
        return 1;
    }
}