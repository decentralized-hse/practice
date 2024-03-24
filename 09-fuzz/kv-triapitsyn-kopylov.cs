using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.IO;
using System.Linq;
using System.Text;

public class MainParser {
  public struct Project {
    public byte[] repo;
    public byte mark;
  }

  public struct Student {
    public byte[] name;
    public byte[] login;
    public byte[] group;
    public byte[] practice;
    public Project project;
    public float mark;
  }

  public static List<Student> FromBinFile(string filepath) {
    using (BinaryReader reader = new BinaryReader(File.Open(filepath, FileMode.Open))) {
        return FromBinFileStream(reader);
    }
  }

  public static bool CheckFormat(Student student) {
    try
    {
        Regex regexForName = new Regex(@"^[\wа-яА-Я\s]+\0+$");
        if (!regexForName.IsMatch(Encoding.UTF8.GetString(student.name))) {
          Console.WriteLine("Student name:" + System.Text.Encoding.UTF8.GetString(student.name));
          return false;
        }
        Regex regexForLogin = new Regex(@"^[\w]+\0+$");
        if (!regexForLogin.IsMatch(System.Text.Encoding.UTF8.GetString(student.login))) {
            Console.WriteLine("!neededRegex.IsMatch(student.login)");
            Console.WriteLine("|" + System.Text.Encoding.UTF8.GetString(student.login) + "|");
            return false;
        }
        Regex regexForGroup = new Regex(@"^[\wа-яА-Я\-]+\0+$");
        if (!regexForGroup.IsMatch(System.Text.Encoding.UTF8.GetString(student.group))) {
            Console.WriteLine("!neededRegex.IsMatch(student.group)");
            Console.WriteLine(System.Text.Encoding.UTF8.GetString(student.group));
            return false;
        }

        Regex regexForURL = new Regex(@"^[\w\-/\\_\.]+\0+$");
        if (!regexForURL.IsMatch(System.Text.Encoding.UTF8.GetString(student.project.repo))) {
            Console.WriteLine("!regexURL.IsMatch(student.project.repo)");
            Console.WriteLine(System.Text.Encoding.UTF8.GetString(student.project.repo));
            return false;
        }
    }
    catch (DecoderFallbackException)
    {
        Console.WriteLine("The byte array does not represent a valid UTF-8 string.");
        return false;
    }

    for (int prac = 0; prac < student.practice.Length; prac++) {
      if ((student.practice[prac] != 0) && (student.practice[prac] != 1)) {
        Console.WriteLine("(student.practice[prac] != 0) && (student.practice[prac] != 1)");
        return false;
      }
    }
    if (student.project.mark < 0 || student.project.mark > 10) {
      Console.WriteLine("(student.project.mark < 0 || student.project.mark > 10)");
      return false;
    }
    if (student.mark < 0 || student.mark > 10 || Double.IsNaN(student.mark)) {
      Console.WriteLine("student.mark < 0 || student.mark > 10 || IsNaN(student.mark)");
      return false;
    }
    Console.WriteLine("CheckFormat successful");
    return true;
  }

  public static List<Student> FromBinFileStream(BinaryReader reader, bool createInitialFile = false) {
    List<Student> students = new List<Student>();
    BinaryWriter writer = null;
    if (createInitialFile) {
        writer = new BinaryWriter(File.Open("test_initial.bin", FileMode.Create));
    }

    bool atLeastOneFullStudent = false;
    bool lenOfBytesIsZero = false;
    while (true) {
    try {
        Console.WriteLine("New loop from FromBinFileStream");
        Student stud = new Student();
        byte[] readingRes = reader.ReadBytes(32);
        if (readingRes.Length < 32) {
          if (readingRes.Length == 0) {
            lenOfBytesIsZero = true;
          }
          throw new EndOfStreamException();
        }
        stud.name = readingRes;
        Console.WriteLine("after name");
        stud.login = reader.ReadBytes(16);
        Console.WriteLine("after login");
        stud.group = reader.ReadBytes(8);
        Console.WriteLine("after group");
        stud.practice = reader.ReadBytes(8);
        Console.WriteLine("after practice");
        stud.project.repo = reader.ReadBytes(59);
        Console.WriteLine("after repo");
        stud.project.mark = reader.ReadByte();

        Console.WriteLine("after mark");

        // check for float in case of ZERO
        Console.WriteLine("Reading float");
        byte[] bytesForFloat = reader.ReadBytes(4);

        if (bytesForFloat.Length < 4) {
          throw new FormatException("CheckFormat failed for student");
        }
        Console.WriteLine("after final mark");
        try {
          stud.mark = BitConverter.ToSingle(bytesForFloat);
        }
        catch (Exception ex) {
          Console.WriteLine("REAL REASON is " + ex.Message);
          throw new FormatException("CheckFormat failed for student");
        }
        atLeastOneFullStudent = true;
        
        Console.WriteLine($"mark is {stud.mark}");
        if ((float)Math.Round(stud.mark, 6) == 0.0f) {
          foreach (byte b in bytesForFloat) {
            Console.WriteLine("checking byte");
            if (b != 0) {
              Console.WriteLine("byte NOT OK");
              throw new FormatException("CheckFormat failed for student");
            }
            Console.WriteLine("byte OK");
          }
        }

        if (createInitialFile) {
            writer.Write(stud.name);
            writer.Write(stud.login);
            writer.Write(stud.group);
            writer.Write(stud.practice);
            writer.Write(stud.project.repo);
            writer.Write(stud.project.mark);
            writer.Write(stud.mark);
        }
        // проверка на соответствие формату
        if (!CheckFormat(stud)) {
          throw new FormatException("CheckFormat failed for student");
        }
        students.Add(stud);
    } catch (EndOfStreamException) { 
        if (atLeastOneFullStudent && lenOfBytesIsZero) {
          // can exit cleanly
          break;
        } else {
          throw new FormatException("CheckFormat failed for student");
        }
    }
    }

    if (createInitialFile) {
        writer.Close();
    }
    return students;
  }
  

  public static List<Student> FromKVFile(string filepath) { 
    Console.WriteLine("FromKVFile(string filepath)");

    using (StreamReader reader = new StreamReader(File.Open(filepath, FileMode.Open))) {
        return FromKVFileStream(reader);
    }
  }

  public static void PrintFile(string file) {
    Console.WriteLine("Started printing " + file);
    using (StreamReader reader = new StreamReader(file))
            {
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    Console.WriteLine(line);
                }
            }
    Console.WriteLine("Done printing \n" + file);
  }

  public static bool CompareFiles(string afterTransformations = "test_file.kv", string initial = "test_initial.kv") {
        byte[] file1Bytes = File.ReadAllBytes(afterTransformations);
        byte[] file2Bytes = File.ReadAllBytes(initial);
        if (file1Bytes.Length != file2Bytes.Length)
        {
            Console.WriteLine("Files are not equal 1");

            PrintFile(initial);
            PrintFile(afterTransformations);

            return false;
        }

        for (int i = 0; i < file1Bytes.Length; i++)
        {
            if (file1Bytes[i] != file2Bytes[i])
            {
                Console.WriteLine("Files are not equal 2 ");

                PrintFile(initial);
                PrintFile(afterTransformations);

                return false;
            }
        }
        Console.WriteLine("Files are equal 3");
        return true;
  }

  public static List<Student> FromKVFileStream(StreamReader reader, bool createInitialFile = false) {
    string exceptionMessage = "Cannot parse from FromKVFileStream";
    Console.WriteLine("FromKVFileStream");

    StreamWriter writer = null;
    var students = new List<Student>();
    if (createInitialFile) {
        writer = new StreamWriter(File.Open("test_initial.kv", FileMode.Create));
    }
    const string NAME = "name";
    const string LOGIN = "login";
    const string GROUP = "group";
    const string PRACTICE = "practice";
    const string PROJECT_REPO = "project_repo";
    const string PROJECT_MARK = "project_mark";
    const string MARK = "mark";

    string expecting = NAME;
    int expectingPracticeNum = 0;
    Console.WriteLine("Before try");
    try {
      while (!reader.EndOfStream) {
      Console.WriteLine("Before reading line");
      string line = reader.ReadLine();
      string[] kv = line.Split(' ');
      Console.WriteLine("Line is " + line);
      string key = kv[0];
      string value = string.Join(" ", kv.Skip(1));
      
      Match matchName = Regex.Match(key, @"^\[(\d+)\]\.name:$");
      Match matchLogin = Regex.Match(key, @"^\[(\d+)\]\.login:$");
      Match matchGroup = Regex.Match(key, @"^\[(\d+)\]\.group:$");
      Match matchPractice = Regex.Match(key, @"^\[(\d+)\]\.practice\.\[(\d+)\]:$");
      Match matchProjectRepo = Regex.Match(key, @"^\[(\d+)\]\.project\.repo:$");
      Match matchProjectMark = Regex.Match(key, @"^\[(\d+)\]\.project\.mark:$");
      Match matchMark = Regex.Match(key, @"^\[(\d+)\]\.mark:$");

      Console.WriteLine("Before matchName.Success");

      if (matchName.Success) {
          if (expecting != NAME) {
            throw new FormatException(exceptionMessage);
          }
          expecting = LOGIN;
          Console.WriteLine("matchName.Success");
          int id;
          bool success = int.TryParse(matchName.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id != students.Count) {
            throw new FormatException(exceptionMessage);
          }
          students.Add(new Student());
          Student stud = students[id];
          Regex regexForName = new Regex(@"^[\w\0а-яА-Я\s]+$");
          stud.name = System.Text.Encoding.UTF8.GetBytes(value);
          System.Array.Resize(ref stud.name, 32);
          if (!regexForName.IsMatch(Encoding.UTF8.GetString(stud.name))) {
            throw new FormatException(exceptionMessage);
          }
          students[id] = stud;

      } else if (matchLogin.Success) {
          if (expecting != LOGIN) {
            throw new FormatException(exceptionMessage);
          }
          expecting = GROUP;
          Console.WriteLine("matchLogin.Success");
          int id;
          bool success = int.TryParse(matchLogin.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id > students.Count || id < 0) {
            throw new FormatException(exceptionMessage);
          }
          Student stud = students[id];
          stud.login = System.Text.Encoding.UTF8.GetBytes(value);

          Regex regexForLogin = new Regex(@"^[\w\0]+$");
          System.Array.Resize(ref stud.login, 16);
          if (!regexForLogin.IsMatch(System.Text.Encoding.UTF8.GetString(stud.login))) {
            throw new FormatException();
          }
          students[id] = stud;

      } else if (matchGroup.Success) {
          if (expecting != GROUP) {
            throw new FormatException(exceptionMessage);
          }
          expecting = PRACTICE;
          Console.WriteLine("matchGroup.Success");
          int id;
          bool success = int.TryParse(matchGroup.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id > students.Count || id < 0) {
            throw new FormatException(exceptionMessage);
          }
          Student stud = students[id];
          Regex regexForGroup = new Regex(@"^[\w\0а-яА-Я\-]+$");
          stud.group = System.Text.Encoding.UTF8.GetBytes(value);
          System.Array.Resize(ref stud.group, 8);
          if (!regexForGroup.IsMatch(System.Text.Encoding.UTF8.GetString(stud.group))) {
            throw new FormatException(exceptionMessage);
          }
          students[id] = stud;

      } else if (matchPractice.Success) {
          if (expecting != PRACTICE) {
            throw new FormatException(exceptionMessage);
          }
          Console.WriteLine("matchPractice.Success");
          int id;
          bool success = int.TryParse(matchPractice.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id > students.Count || id < 0) {
            throw new FormatException(exceptionMessage);
          }
          int practiceID;
          success = int.TryParse(matchPractice.Groups[2].Value, out practiceID);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (practiceID >= 8 || practiceID < 0) {
            throw new FormatException(exceptionMessage);
          }
          if (practiceID != expectingPracticeNum) {
            throw new FormatException(exceptionMessage);
          }
          expectingPracticeNum = expectingPracticeNum + 1;
          if (expectingPracticeNum == 8) {
            expectingPracticeNum = 0;
            expecting = PROJECT_REPO;
          }
          Student stud = students[id];
          if (practiceID == 0) {
              stud.practice = new byte[8];
          }
          byte[] prac = stud.practice;
          int parsing_res;
          success = int.TryParse(value, out parsing_res);
          if (!success || !(parsing_res == 0 || parsing_res == 1)) {
            throw new FormatException(exceptionMessage);
          }
          prac[practiceID] = (byte)parsing_res;
          stud.practice = prac;
          students[id] = stud;

      } else if (matchProjectRepo.Success) {
          if (expecting != PROJECT_REPO) {
            throw new FormatException(exceptionMessage);
          }
          expecting = PROJECT_MARK;
          Console.WriteLine("matchProjectRepo.Success");
          int id;
          bool success = int.TryParse(matchProjectRepo.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id > students.Count || id < 0) {
            throw new FormatException(exceptionMessage);
          }
          Student stud = students[id];
          stud.project.repo = System.Text.Encoding.UTF8.GetBytes(value);
          Regex regexForURL = new Regex(@"^[\w\0\-/\\_\.]+$");
          System.Array.Resize(ref stud.project.repo, 59);
          if (!regexForURL.IsMatch(System.Text.Encoding.UTF8.GetString(stud.project.repo))) {
            throw new FormatException(exceptionMessage);
          }
          students[id] = stud;
      } else if (matchProjectMark.Success) {
          if (expecting != PROJECT_MARK) {
            throw new FormatException(exceptionMessage);
          }
          expecting = MARK;
          Console.WriteLine("matchProjectMark.Success");
          int id; 
          bool success = int.TryParse(matchProjectMark.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id > students.Count || id < 0) {
            throw new FormatException(exceptionMessage);
          }
          Student stud = students[id];
          success = byte.TryParse(value, out stud.project.mark);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          students[id] = stud;

      } else if (matchMark.Success) {
          if (expecting != MARK) {
            throw new FormatException(exceptionMessage);
          }
          expecting = NAME;
          Console.WriteLine("matchMark.Success");
          int id;
          bool success = int.TryParse(matchMark.Groups[1].Value, out id);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          if (id > students.Count || id < 0) {
            throw new FormatException(exceptionMessage);
          }
          Student stud = students[id];
          success = float.TryParse(value, out stud.mark);
          if (!success) {
            throw new FormatException(exceptionMessage);
          }
          students[id] = stud;
      
      } else {
          Console.WriteLine("BEFORE throw new FormatException(exceptionMessage);");
          throw new FormatException(exceptionMessage);
      }
      if (createInitialFile) {
        writer.WriteLine(line);
      }
      }
    }
    catch (Exception ex) {
      Console.WriteLine("REAL exception is: " + ex.Message);
      throw new FormatException(exceptionMessage);
    }

    if (createInitialFile) {
        writer.Close();
    }

    if (expecting != NAME) {
      throw new FormatException(exceptionMessage);
    }

    return students;
  }

  public static void ToBinFile(List<Student> students, string filepath) {
    using(BinaryWriter writer = new BinaryWriter(File.Open(filepath, FileMode.Create))) {
      foreach(Student stud in students) {
        writer.Write(stud.name);
        writer.Write(stud.login);
        writer.Write(stud.group);
        writer.Write(stud.practice);
        writer.Write(stud.project.repo);
        writer.Write(stud.project.mark);
        writer.Write(stud.mark);
      }
    }
  }
  
  static string StringFromBytes(byte[] bytes) {
    int index = Array.IndexOf(bytes, (byte) 0);
    if (index < 0) {
      index = bytes.Length;
    }
    return System.Text.Encoding.UTF8.GetString(bytes.Take(index).ToArray());
  }
  
  
  public static void ToKVFile(List<Student> students, string filepath) {
    using(StreamWriter writer = new StreamWriter(File.Open(filepath, FileMode.Create))) {
      for (int i = 0; i < students.Count; i++) {
        writer.WriteLine($"[{i}].name: {StringFromBytes(students[i].name)}");
        writer.WriteLine($"[{i}].login: {StringFromBytes(students[i].login)}");
        writer.WriteLine($"[{i}].group: {StringFromBytes(students[i].group)}");
        for (int prac = 0; prac < students[i].practice.Length; prac++) {
          writer.WriteLine($"[{i}].practice.[{prac}]: {students[i].practice[prac]}");
        }
        writer.WriteLine($"[{i}].project.repo: {StringFromBytes(students[i].project.repo)}");
        writer.WriteLine($"[{i}].project.mark: {students[i].project.mark}");
        writer.WriteLine($"[{i}].mark: {(float)Math.Round(students[i].mark, 6)}");
      }
    }
  }


  public static int ProcessBinary(string filepath, bool createInitialFile = false) {
    using (BinaryReader reader = new BinaryReader(File.Open(filepath, FileMode.Open))) {
        return ProcessBinaryStream(reader, filepath, createInitialFile);
    }
  }

  public static int ProcessBinaryStream(BinaryReader reader, string initialInputnputFile, bool createInitialFile = false) {
    try {
      Console.WriteLine($"Reading binary file {initialInputnputFile}");
      List<Student> students = FromBinFileStream(reader, createInitialFile);
      Console.WriteLine($"{students.Count} records found.");
      string outputFile = Path.ChangeExtension(initialInputnputFile, ".kv");
      Console.WriteLine($"Writing to key-value file {outputFile}");
      ToKVFile(students, outputFile);
    }
    catch (Exception ex) {
      if (ex.Message == "CheckFormat failed for student") {
        return -1;
      } else {
        throw ex;
      }
    }
    return 0;
  }

  public static int ProcessKV(string filepath, bool createInitialFile = false) {
    using (StreamReader reader = new StreamReader(File.Open(filepath, FileMode.Open))) {
        return ProcessKVStream(reader, filepath, createInitialFile);
    }
  }


  public static int ProcessKVStream(StreamReader reader, string initialInputnputFile, bool createInitialFile = false) {
    try {
      Console.WriteLine($"Reading key-value file {initialInputnputFile}");
      List<Student> students = FromKVFileStream(reader, createInitialFile);
      
      Console.WriteLine($"{students.Count} records found.");
      string outputFile = Path.ChangeExtension(initialInputnputFile, ".bin");
      
      Console.WriteLine($"Writing to binary file {outputFile}");
      ToBinFile(students, outputFile);
    }
    catch (Exception ex) {
      if (ex.Message == "CheckFormat failed for student" || ex.Message == "Cannot parse from FromKVFileStream") {
        return -1;
      } else {
        throw ex;
      }
    }
    return 0;
  }

  public static int Main(string[] args) {
    if (args.Length < 1) {
      Console.WriteLine("Provide a file as an argument");
      return -1;
    }

    string inputFile = args[0];
    string inputFormat = Path.GetExtension(inputFile);
    int res = -2;
    if (inputFormat == ".bin") {
      Console.WriteLine("Wants to process .bin file");
      res = ProcessBinary(inputFile);
    } else if (inputFormat == ".kv") {
      Console.WriteLine("Wants to process .kv file");
      res = ProcessKV(inputFile);
    }

    if (res == 0) {
      Console.WriteLine("Processed file successfully");
    } else if (res == -1) {
      TextWriter errorWriter = Console.Error;
      errorWriter.WriteLine("Malformed input");
    } else {
      Console.WriteLine("Something strange is going on!");
    }
    return res;
  }
}
