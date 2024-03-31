using System.Runtime.InteropServices;
using System;
using System.IO;
using System.Text;
using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Unicode;

public class ParseException : Exception {
    public ParseException(string message) : base(message) {}
}

public class JsonParseLibrary
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Project
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 59)]
        public byte[] name;

        public string Name
        {
            get { return BytesToString(name); }
            set { name = StringToBytes(value, 59); }
        }

        [MarshalAs(UnmanagedType.U1)] public Byte mark;

        public Byte Mark
        {
            get { return mark; }
            set { mark = value; }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Student
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] name;

        public string Name
        {
            get { return BytesToString(name); }
            set { name = StringToBytes(value, 32); }
        }

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public byte[] login;

        public string Login
        {
            get { return BytesToString(login); }
            set { login = StringToBytes(value, 16); }
        }

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public byte[] group;

        public string Group
        {
            get { return BytesToString(group); }
            set { group = StringToBytes(value, 8); }
        }

        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.U1, SizeConst = 8)]

        public byte[] practice;

        public bool[] Practice
        {
            get { return (from bit in practice select Convert.ToBoolean(bit)).ToArray(); }
            set { practice = (from bit in value select bit ? (byte)1 : (byte)0).ToArray(); }
        }

        public Project project;

        public Project Project
        {
            get { return project; }
            set { project = value; }
        }

        public float mark;

        public float Mark
        {
            get { return mark; }
            set { mark = value; }
        }
    }

    public struct Students
    {
        public List<Student> StudentsList { get; set; }
    }

    public static byte[] StringToBytes(String str, int size)
    {
        var bytes = Encoding.UTF8.GetBytes(str);
        Array.Resize(ref bytes, size);
        return bytes;
    }

    public static String BytesToString(byte[] bytes)
    {
        int lastIndex = Array.FindLastIndex(bytes, b => b != 0);
        Array.Resize(ref bytes, lastIndex + 1);
        return Encoding.UTF8.GetString(bytes);
    }

    public static void ValidateStudentInfo(Student student, string str) {
        if (student.Name.Length > 32) {
            throw new ParseException($"Invalid {str} input: name.Length > 32 ({student.Name.Length})");
        }
        if (student.Login.Length > 16) {
            throw new ParseException($"Invalid {str} input: login.Length > 16 ({student.Login.Length})");
        }
        if (student.Group.Length > 8) {
            throw new ParseException($"Invalid {str} input: group.Length > 8 ({student.Group.Length})");
        }
        if (float.IsNaN(student.Mark) || float.IsInfinity(student.Mark)) {
            throw new ParseException($"Invalid {str} input: invalid mark floating-point value");
        }
        if (student.Practice.Length != 8) {
            throw new ParseException($"Invalid {str} input: practice.Length != 8 ({student.Practice.Length})");
        }
        for (int i = 0; i < student.Practice.Length; ++i) {
            if (student.practice[i] != 0x00 && student.practice[i] != 0x01) {
                throw new ParseException($"Invalid {str} input: invalid boolean byte project.practice value {student.practice[i]}");
            }
        }
        if (student.Project.Name.Length > 59) {
            throw new ParseException($"Invalid {str} input: project.name.Length > 59 ({student.Project.Name.Length})");
        }
        if (!student.Name.All(c => char.IsLetterOrDigit(c) || char.IsPunctuation(c) || char.IsWhiteSpace(c)) || student.Name.Any(c => char.IsControl(c))) {
            throw new ParseException($"Invalid {str} input: name is not readable");
        }
        if (!student.Login.All(c => char.IsLetterOrDigit(c) || char.IsPunctuation(c) || char.IsWhiteSpace(c)) || student.Login.Any(c => char.IsControl(c))) {
            throw new ParseException($"Invalid {str} input: login is not readable");
        }
        if (!student.Project.Name.All(c => char.IsLetterOrDigit(c) || char.IsPunctuation(c) || char.IsWhiteSpace(c)) || student.Project.Name.Any(c => char.IsControl(c))) {
            throw new ParseException($"Invalid {str} input: project.name is not readable");
        }
        if (!student.Group.All(c => char.IsLetterOrDigit(c) || char.IsPunctuation(c) || char.IsWhiteSpace(c)) || student.Group.Any(c => char.IsControl(c))) {
            throw new ParseException($"Invalid {str} input: group is not readable");
        }
        if (Marshal.SizeOf(student) != 128) {
            throw new ParseException($"Invalid {str} input: wrong student size({Marshal.SizeOf(student)} != 128)");
        }
    }
}

public class FilesComporator {
    public static bool CompareBinFiles(string lhs, string rhs) {
        byte[] lhsBytes = File.ReadAllBytes(lhs);
        byte[] rhsBytes = File.ReadAllBytes(rhs);
        if (lhsBytes.Length != rhsBytes.Length) {
            Console.WriteLine("Files are not equal (by length:" + lhsBytes.Length + ", " + rhsBytes.Length + ")");
            return false;
        }
        for (int i = 0; i < lhsBytes.Length; ++i) {
            if (lhsBytes[i] != rhsBytes[i]) {
                Console.WriteLine("Files are not equal (by bytes: " + JsonParseLibrary.BytesToString(lhsBytes) + ", " + JsonParseLibrary.BytesToString(rhsBytes) + ")");
                return false;
            }
        }
        Console.WriteLine("Files are equal");
        return true;
    }
    public static bool CompareJsonFiles(string lhs, string rhs) {
        var options = new JsonSerializerOptions
        {
            Encoder = JavaScriptEncoder.Create(UnicodeRanges.All),
            WriteIndented = true
        };
        var lbytes = File.ReadAllBytes(lhs);
        var rbytes = File.ReadAllBytes(rhs);
        var lstudents = JsonSerializer.Deserialize<JsonParseLibrary.Students>(lbytes, options);
        var rstudents = JsonSerializer.Deserialize<JsonParseLibrary.Students>(rbytes, options);

        if (lstudents.StudentsList.Count != rstudents.StudentsList.Count) {
            Console.WriteLine("Files are not equal (by length" + lstudents.StudentsList.Count + ", " + rstudents.StudentsList.Count + ")");
            return false;
        }
        for (int i = 0; i < lstudents.StudentsList.Count; ++i) {
            if (lstudents.StudentsList[i].Name != rstudents.StudentsList[i].Name) {
                Console.WriteLine("Files are not equal (by name: " + lstudents.StudentsList[i].Name + ", " + rstudents.StudentsList[i].Name + ")");
                return false;
            }
            if (lstudents.StudentsList[i].Login != rstudents.StudentsList[i].Login) {
                Console.WriteLine("Files are not equal (by login: " + lstudents.StudentsList[i].Login + ", " + rstudents.StudentsList[i].Login + ")");
                return false;
            }
            if (lstudents.StudentsList[i].Group != rstudents.StudentsList[i].Group) {
                Console.WriteLine("Files are not equal (by group: " + lstudents.StudentsList[i].Group + ", " + rstudents.StudentsList[i].Group + ")");
                return false;
            }
            if (!lstudents.StudentsList[i].practice.SequenceEqual(rstudents.StudentsList[i].practice)) {
                Console.WriteLine("Files are not equal (by practice: [" + String.Join(", ", lstudents.StudentsList[i].practice) + "], [" + String.Join(", ", rstudents.StudentsList[i].practice) + "])");
                return false;
            }
            if (lstudents.StudentsList[i].Project.Name != rstudents.StudentsList[i].Project.Name) {
                Console.WriteLine("Files are not equal (by project.name: " + lstudents.StudentsList[i].Project.Name + ", " + rstudents.StudentsList[i].Project.Name + ")");
                return false;
            }
            if (lstudents.StudentsList[i].Project.Mark != rstudents.StudentsList[i].Project.Mark) {
                Console.WriteLine("Files are not equal (by project.mark: " + lstudents.StudentsList[i].Project.Mark + ", " + rstudents.StudentsList[i].Project.Mark + ")");
                return false;
            }
            if (lstudents.StudentsList[i].Mark != rstudents.StudentsList[i].Mark) {
                Console.WriteLine("Files are not equal (by mark: " + lstudents.StudentsList[i].Mark + ", " + rstudents.StudentsList[i].Mark + ")");
                return false;
            }
        }
        Console.WriteLine("Files are equal");
        return true;
    }
}
public class JsonParseProgram
{

    static JsonParseLibrary.Students ReadBin(String path)
    {
        if (new FileInfo(path).Length % Marshal.SizeOf<JsonParseLibrary.Student>() != 0) {
            throw new ParseException($"Invalid binary input: not aligned to {Marshal.SizeOf<JsonParseLibrary.Student>()}");
        }
        var students = new JsonParseLibrary.Students{StudentsList = new List<JsonParseLibrary.Student> ()};
        using (var stream = File.Open(path, FileMode.Open))
        {
            using (var reader = new BinaryReader(stream, Encoding.UTF8, false))
            {
                while (true)
                {
                    var bytes = reader.ReadBytes(Marshal.SizeOf<JsonParseLibrary.Student>());
                    if (bytes.Length != Marshal.SizeOf<JsonParseLibrary.Student>())
                    {
                        break;
                    }
                    GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
                    var student = (JsonParseLibrary.Student)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(JsonParseLibrary.Student));
                    students.StudentsList.Add(student);
                    handle.Free();
                }
            }
        }

        foreach (var student in students.StudentsList)
        {
            JsonParseLibrary.ValidateStudentInfo(student, "binary");
        }

        return students;
    }

    static void WriteBin(JsonParseLibrary.Students students, String path)
    {

        using (var stream = File.Open(path, FileMode.Create))
        {
            foreach (var student in students.StudentsList)
            {
                var size = Marshal.SizeOf(student);
                var pnt = Marshal.AllocHGlobal(size);
                Marshal.StructureToPtr(student, pnt, false);
                byte[] buffer = new byte[size];
                Marshal.Copy(pnt, buffer, 0, size);
                stream.Write(buffer);
                Marshal.FreeHGlobal(pnt);     
            }
        }
    }

    static void WriteJson(JsonParseLibrary.Students students, String path)
    {
        var options = new JsonSerializerOptions
        {
            Encoder = JavaScriptEncoder.Create(UnicodeRanges.All),
            WriteIndented = true,
            AllowTrailingCommas = false
        };
        string jsonString = JsonSerializer.Serialize(students, options);
        File.WriteAllText(path, jsonString);
    }

    static JsonParseLibrary.Students ReadJson(String path)
    {
        var options = new JsonSerializerOptions
        {
            Encoder = JavaScriptEncoder.Create(UnicodeRanges.All),
            WriteIndented = true,
            AllowTrailingCommas = false
        };
        var bytes = File.ReadAllBytes(path);
        var students = new JsonParseLibrary.Students{StudentsList = new List<JsonParseLibrary.Student> ()};
        try {
            students = JsonSerializer.Deserialize<JsonParseLibrary.Students>(bytes, options);
        } catch (Exception ex) {
            if (ex is System.InvalidOperationException || ex is JsonException) {
                throw new ParseException($"Invalid json input: {ex.Message}");
            } else {
                throw;
            }
        }
        try {
            foreach (var student in students.StudentsList)
            {
                JsonParseLibrary.ValidateStudentInfo(student, "json");
            }
        } catch (Exception ex) {
            if (ex is System.ArgumentNullException || ex is System.NullReferenceException) {
                throw new ParseException($"Invalid json input: {ex.Message}");
            } else {
                throw;
            }
        }
        return students;
    }

    public static void BinToJson(String input_path, String output_path)
    {
        var students = ReadBin(input_path);
        WriteJson(students, output_path);
    }

    public static void JsonToBin(String input_path, String output_path)
    {
        var students = ReadJson(input_path);
        WriteBin(students, output_path);
    }

    static void Main(string[] args)
    {
        if (args.Length == 1) {
            String path = args[0];
            if (Path.GetExtension(path) == ".bin")
            {
                BinToJson(path, Path.ChangeExtension(path, ".json"));
            }
            else if (Path.GetExtension(path) == ".json")
            {
                JsonToBin(path, Path.ChangeExtension(path, ".bin"));
            }
            else
            {
                Console.WriteLine("Unexpected extension of file: " + Path.GetExtension(path));
                Environment.Exit(1);
            }
        } else if (args.Length == 2) {
            if (args[0] == "test") {
                String path = args[1];
                if (Path.GetExtension(path) == ".bin")
                {
                    BinToJson(path, "result.json");
                    JsonToBin("result.json", "result.bin");
                    FilesComporator.CompareBinFiles(path, "result.bin");
                }
                else if (Path.GetExtension(path) == ".json")
                {
                    JsonToBin(path, "result.bin");
                    BinToJson("result.bin", "result.json");
                    FilesComporator.CompareJsonFiles(path, "result.json");
                }
                else
                {
                    Console.WriteLine("Unexpected extension of file: " + Path.GetExtension(path));
                    Environment.Exit(1);
                }
                Environment.Exit(0);
            }
            String lpath = args[0];
            String rpath = args[1];
            if (Path.GetExtension(lpath) == ".bin" && Path.GetExtension(rpath) == ".bin") {
                FilesComporator.CompareBinFiles(lpath, rpath);
            } else if (Path.GetExtension(lpath) == ".json" && Path.GetExtension(rpath) == ".json") {
                FilesComporator.CompareJsonFiles(lpath, rpath);
            } else {
                Console.WriteLine("Unexpected extensions of file: " + Path.GetExtension(lpath) + ", " + Path.GetExtension(rpath));
                Environment.Exit(1);
            }
        } else {
            Console.WriteLine("Unexpected amount of aguments: " + args.Length);
            Environment.Exit(1);
        }
    }
}
