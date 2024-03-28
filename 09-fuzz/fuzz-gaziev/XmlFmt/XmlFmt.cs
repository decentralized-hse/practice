﻿using System;
using System.IO;
using System.Text;
using System.Xml;
using System.Xml.Serialization;

namespace XmlFmt {

public class ParseException : Exception {
    public ParseException(string message) : base(message) {}
}

public class Student {
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

public class FilesComporator {
    public static bool CompareBinFiles(string lhs, string rhs) {
        byte[] lhsBytes = File.ReadAllBytes(lhs);
        byte[] rhsBytes = File.ReadAllBytes(rhs);
        if (lhsBytes.Length != rhsBytes.Length) {
            Console.WriteLine("[ERROR] Files are not equal (by length)");
            return false;
        }
        for (int i = 0; i < lhsBytes.Length; ++i) {
            if (lhsBytes[i] != rhsBytes[i]) {
                Console.WriteLine("[ERROR] Files are not equal (by bytes)");
                return false;
            }
        }
        Console.WriteLine("[INFO] Files are equal");
        return true;
    }

    public static bool CompareXmlFiles(string lhs, string rhs) {
        XmlSerializer ser = new XmlSerializer(typeof(Student[]));

        Student[] lhsArr = new Student[0];
        using (Stream reader = new FileStream(lhs, FileMode.Open)) {
            var deserialized = ser.Deserialize(reader);
            if (deserialized != null) {
                lhsArr = (Student[]) deserialized;
            }
        }

        Student[] rhsArr = new Student[0];
        using (Stream reader = new FileStream(rhs, FileMode.Open)) {
            var deserialized = ser.Deserialize(reader);
            if (deserialized != null) {
                rhsArr = (Student[]) deserialized;
            }
        }

        if (lhsArr.Length != rhsArr.Length) {
            Console.WriteLine("[ERROR] Files are not equal (by length)");
            return false;
        }
        for (uint i = 0; i < lhsArr.Length; ++i) {
            bool isEqual = (
                lhsArr[i].name == rhsArr[i].name &&
                lhsArr[i].login == rhsArr[i].login &&
                lhsArr[i].group == rhsArr[i].group &&
                lhsArr[i].practice.SequenceEqual(rhsArr[i].practice) &&
                lhsArr[i].project.repo == rhsArr[i].project.repo &&
                lhsArr[i].project.mark == rhsArr[i].project.mark &&
                lhsArr[i].mark == rhsArr[i].mark
            );
            if (!isEqual) {
                Console.WriteLine("[ERROR] Files are not equal (by content)");
                return false;
            }
        }
        Console.WriteLine("[INFO] Files are equal");
        return true;
    }

}

public class XmlFmtProgram {
    static byte[] StrToPaddedBytes(string str, uint size) {
        byte[] res = new byte[size];
        byte[] strBytes = System.Text.Encoding.UTF8.GetBytes(str);
        if (strBytes.Length > size) {
            throw new ParseException("Malformed input в stderr: the bytes size for string data is more than allowed");
        }
        for (uint i = 0; i < strBytes.Length & i < size; ++i) {
            res[i] = strBytes[i];
        }
        for (uint i = (uint)strBytes.Length; i < size; ++i) {
            res[i] = 0;
        }
        return res;
    }

    static void IsHumanReadable(string input) {
        if (input.Trim().Length == 0) {
            throw new ParseException("Malformed input в stderr: Trimmed string input is empty");
        }
        if (input.Any(c => char.IsControl(c))) {
            throw new ParseException("Malformed input в stderr: String input is not readable");
        }
        if (!input.All(c => char.IsLetterOrDigit(c) || char.IsPunctuation(c) || char.IsWhiteSpace(c))) {
            throw new ParseException("Malformed input в stderr: String input is not readable");
        }
    }

    static void IsPureBoolean(byte bt) {
        if (bt != 0x00 && bt != 0x01) {
            throw new ParseException("Malformed input в stderr: Boolean byte is not equal to 0x00 or 0x01");
        }
    }

    static void IsValidFloat(float value) {
        if (float.IsNaN(value) || float.IsInfinity(value)) {
            throw new ParseException("Malformed input в stderr: invalid floating-point value");
        }
    }

    public static void BinToXml(string inFilename, string outFilename) {
        Console.WriteLine($"[INFO] Reading binary student data from {inFilename}");

        if (new FileInfo(inFilename).Length % 128 != 0) {
            throw new ParseException("Malformed input в stderr: Binary input is not aligned to 128");
        }

        long numStudents = new FileInfo(inFilename).Length / 128;
        Student[] studentArr = new Student[numStudents];

        using (var stream = File.Open(inFilename, FileMode.Open)) {
            using (var reader = new BinaryReader(stream, Encoding.UTF8, false)) {
                for (uint i = 0; i < numStudents; ++i) {
                    studentArr[i] = new Student();
                    studentArr[i].name  = Encoding.UTF8.GetString(reader.ReadBytes(32)).TrimEnd('\0');
                    studentArr[i].login = Encoding.UTF8.GetString(reader.ReadBytes(16)).TrimEnd('\0');
                    studentArr[i].group = Encoding.UTF8.GetString(reader.ReadBytes(8)).TrimEnd('\0');

                    IsHumanReadable(studentArr[i].name);
                    IsHumanReadable(studentArr[i].login);
                    IsHumanReadable(studentArr[i].group);

                    byte[] practice = reader.ReadBytes(8);
                    for (int j = 0; j < 8; ++j) {
                        IsPureBoolean(practice[j]);
                        studentArr[i].practice[j] = Convert.ToBoolean(practice[j]);
                    }
                    studentArr[i].project.repo = Encoding.UTF8.GetString(reader.ReadBytes(59)).TrimEnd('\0');
                    IsHumanReadable(studentArr[i].project.repo);
                    studentArr[i].project.mark = reader.ReadByte();
                    studentArr[i].mark = reader.ReadSingle();
                    IsValidFloat(studentArr[i].mark);
                }
            }
        }
        Console.WriteLine($"[INFO] {numStudents} students read...");

        XmlSerializer ser = new XmlSerializer(typeof(Student[]));
        XmlSerializerNamespaces ns = new XmlSerializerNamespaces();
        ns.Add("","");
        using (var writer = new StreamWriter(outFilename)) {
            ser.Serialize(writer, studentArr, ns);
        }

        Console.WriteLine($"[INFO] Xml is written to {outFilename}");
    }

    public static void XmlToBin(string inFilename, string outFilename) {
        Console.WriteLine($"[INFO] Reading xml student data from {inFilename}");

        XmlSerializer ser = new XmlSerializer(typeof(Student[]));
        Student[] studentArr;

        using (Stream reader = new FileStream(inFilename, FileMode.Open)) {
            try {
            var deserialized = ser.Deserialize(reader);
            if (deserialized == null) {
                studentArr = new Student[0];
            } else {
                studentArr = (Student[]) deserialized;
            }
            } catch (System.InvalidOperationException ex) {
                throw new ParseException($"Malformed input в stderr: {ex.Message}");
            }
            
        }
        Console.WriteLine($"[INFO] {studentArr.Length} students read...");

        using (var stream = File.Open(outFilename, FileMode.Create)) {
            using (var writer = new BinaryWriter(stream, Encoding.UTF8, false)) {
                for (uint i = 0; i < studentArr.Length; ++i) {
                    if (studentArr[i].practice.Length < 8) {
                        throw new ParseException("Malformed input в stderr: broken practice data in xml");
                    }
                    IsHumanReadable(studentArr[i].name);
                    IsHumanReadable(studentArr[i].login);
                    IsHumanReadable(studentArr[i].group);
                    IsHumanReadable(studentArr[i].project.repo);
                    IsValidFloat(studentArr[i].mark);

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

        Console.WriteLine($"[INFO] written to {outFilename}");
    }

    static int Main(string[] args) {
        if (args.Length < 1) {
            Console.WriteLine("Provide a single argument - student data file for transformation");
            return 1;
        }
        if (!File.Exists(args[0])) {
            Console.WriteLine("File does not exist");
            return 1;
        }
        if (args.Length == 2 && args[1] == "cmp") {
            try {
            if (args[0].EndsWith(".bin")) {
                BinToXml(args[0], "out.xml");
                XmlToBin("out.xml", "out.bin");
                FilesComporator.CompareBinFiles(args[0], "out.bin");
                return 0;
            }
            if (args[0].EndsWith(".xml")) {
                XmlToBin(args[0], "out.bin");
                BinToXml("out.bin", "out.xml");
                FilesComporator.CompareXmlFiles(args[0], "out.xml");
                return 0;
            }
            } catch (ParseException ex) {
                Console.WriteLine($"[EXCEPTION] {ex.Message}");
                return 1;
            }
        } else {
            if (args[0].EndsWith(".bin")) {
                BinToXml(args[0], "out.xml");
                return 0;
            }
            if (args[0].EndsWith(".xml")) {
                XmlToBin(args[0], "out.bin");
                return 0;
            }
        }
        return 1;
    }
}

}
