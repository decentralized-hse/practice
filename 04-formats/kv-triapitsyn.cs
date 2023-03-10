using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.IO;
using System.Linq;
using System.Text;

public class Program {
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

  static List<Student> FromBinFile(string filepath) {
    using(BinaryReader reader = new BinaryReader(File.Open(filepath, FileMode.Open))) {
      List<Student> students = new List<Student>();
      while (true) {
        try {
          Student stud = new Student();
          stud.name = reader.ReadBytes(32);
          stud.login = reader.ReadBytes(16);
          stud.group = reader.ReadBytes(8);
          stud.practice = reader.ReadBytes(8);
          stud.project.repo = reader.ReadBytes(59);
          stud.project.mark = reader.ReadByte();
          stud.mark = reader.ReadSingle();
          students.Add(stud);
        } catch (EndOfStreamException) {
          break;
        }
      }

      return students;
    }
  }
  
  static List<Student> FromKVFile(string filepath) {
    using (StreamReader reader = new StreamReader(File.Open(filepath, FileMode.Open))) {
      List<Student> students = new List<Student>();
      while (!reader.EndOfStream) {
        string line = reader.ReadLine();
        string[] kv = line.Split(' ');
        string key = kv[0];
        string value = string.Join(" ", kv.Skip(1));
        
        Match matchName = Regex.Match(key, @"^\[(\d+)\]\.name:$");
        Match matchLogin = Regex.Match(key, @"^\[(\d+)\]\.login:$");
        Match matchGroup = Regex.Match(key, @"^\[(\d+)\]\.group:$");
        Match matchPractice = Regex.Match(key, @"^\[(\d+)\]\.practice\.\[(\d+)\]:$");
        Match matchProjectRepo = Regex.Match(key, @"^\[(\d+)\]\.project\.repo:$");
        Match matchProjectMark = Regex.Match(key, @"^\[(\d+)\]\.project\.mark:$");
        Match matchMark = Regex.Match(key, @"^\[(\d+)\]\.mark:$");

        if (matchName.Success) {
          int id = int.Parse(matchName.Groups[1].Value);
          students.Add(new Student());
          Student stud = students[id];
          stud.name = System.Text.Encoding.UTF8.GetBytes(value);
          System.Array.Resize(ref stud.name, 32);
          students[id] = stud;

        } else if (matchLogin.Success) {
          int id = int.Parse(matchLogin.Groups[1].Value);
          Student stud = students[id];
          stud.login = System.Text.Encoding.UTF8.GetBytes(value);
          System.Array.Resize(ref stud.login, 16);
          students[id] = stud;

        } else if (matchGroup.Success) {
          int id = int.Parse(matchGroup.Groups[1].Value);
          Student stud = students[id];
          stud.group = System.Text.Encoding.UTF8.GetBytes(value);
          System.Array.Resize(ref stud.group, 8);
          students[id] = stud;

        } else if (matchPractice.Success) {
          int id = int.Parse(matchPractice.Groups[1].Value);
          int practiceID = int.Parse(matchPractice.Groups[2].Value);
          Student stud = students[id];
          if (practiceID == 0) {
              stud.practice = new byte[8];
          }
          byte[] prac = stud.practice;
          prac[practiceID] = byte.Parse(value);
          stud.practice = prac;
          students[id] = stud;

        } else if (matchProjectRepo.Success) {
          int id = int.Parse(matchProjectRepo.Groups[1].Value);
          Student stud = students[id];
          stud.project.repo = System.Text.Encoding.UTF8.GetBytes(value);
          System.Array.Resize(ref stud.project.repo, 59);
          students[id] = stud;

        } else if (matchProjectMark.Success) {
          int id = int.Parse(matchProjectMark.Groups[1].Value);
          Student stud = students[id];
          stud.project.mark = byte.Parse(value);
          students[id] = stud;

        } else if (matchMark.Success) {
          int id = int.Parse(matchMark.Groups[1].Value);
          Student stud = students[id];
          stud.mark = float.Parse(value);
          students[id] = stud;
        
        } else {
            throw new FormatException($"Can not parse key \"{key}\"");
        } 
      }
      return students;
    }
  }

  static void ToBinFile(List<Student> students, string filepath) {
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
  
  
  static void ToKVFile(List<Student> students, string filepath) {
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
        writer.WriteLine($"[{i}].mark: {students[i].mark:F3}");
      }
    }
  }

  static void Main(string[] args) {
    if (args.Length < 1) {
      Console.WriteLine("Provide a file as an argument");
      return;
    }

    string inputFile = args[0];
    string inputFormat = Path.GetExtension(inputFile);
    if (inputFormat == ".bin") {
        Console.WriteLine($"Reading binary file {inputFile}");
        List<Student> students = FromBinFile(inputFile);
        
        Console.WriteLine($"{students.Count} records found.");
        string outputFile = Path.ChangeExtension(inputFile, ".kv");
        
        Console.WriteLine($"Writing to key-value file {outputFile}");
        ToKVFile(students, outputFile);
    
    } else if (inputFormat == ".kv") {
        Console.WriteLine($"Reading key-value file {inputFile}");
        List<Student> students = FromKVFile(inputFile);
        
        Console.WriteLine($"{students.Count} records found.");
        string outputFile = Path.ChangeExtension(inputFile, ".bin");
        
        Console.WriteLine($"Writing to binary file {outputFile}");
        ToBinFile(students, outputFile);
    }
  }
}
