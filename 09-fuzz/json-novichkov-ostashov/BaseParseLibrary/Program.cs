using System.Runtime.InteropServices;
using System.Text;
using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Unicode;


class Program
{
    [StructLayout(LayoutKind.Sequential)]
    struct Project
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
    struct Student
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        private byte[] name;

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
        public bool[] practice;

        public bool[] Practice
        {
            get { return practice; }
            set { practice = value; }
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

    struct Students
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

    static Students ReadBin(String path)
    {
        var students = new Students{StudentsList = new List<Student> ()};
        using (var stream = File.Open(path, FileMode.Open))
        {
            using (var reader = new BinaryReader(stream, Encoding.UTF8, false))
            {
                while (true)
                {
                    var bytes = reader.ReadBytes(Marshal.SizeOf<Student>());
                    if (bytes.Length != Marshal.SizeOf<Student>())
                    {
                        break;
                    }
                    GCHandle handle = GCHandle.Alloc(bytes, GCHandleType.Pinned);
                    var student = (Student)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Student));
                    students.StudentsList.Add(student);
                    handle.Free();
                }
            }
        }

        return students;
    }

    static void WriteBin(Students students, String path)
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

    static void WriteJson(Students students, String path)
    {
        var options = new JsonSerializerOptions
        {
            Encoder = JavaScriptEncoder.Create(UnicodeRanges.All),
            WriteIndented = true
        };
        string jsonString = JsonSerializer.Serialize(students, options);
        File.WriteAllText(path, jsonString);
    }

    static Students ReadJson(String path)
    {
        var options = new JsonSerializerOptions
        {
            Encoder = JavaScriptEncoder.Create(UnicodeRanges.All),
            WriteIndented = true
        };
        var bytes = File.ReadAllBytes(path);
        return JsonSerializer.Deserialize<Students>(bytes, options);
    }

    static void Main(string[] args)
    {
        String path = args[0];
        if (Path.GetExtension(path) == ".bin")
        {
            var students = ReadBin(path);
            WriteJson(students, Path.ChangeExtension(path, ".json"));
        }
        else if (Path.GetExtension(path) == ".json")
        {
            var students = ReadJson(path);
            WriteBin(students, Path.ChangeExtension(path, ".bin"));
        }
        else
        {
            Console.WriteLine("Unexpected extension of file: " + Path.GetExtension(path));
            Environment.Exit(1);
        }
    }
}