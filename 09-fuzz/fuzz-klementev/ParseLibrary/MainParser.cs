using System.Text;
using System.Text.RegularExpressions;
using Domain;

namespace ParseLibrary;

public static class MainParser
{
    public static List<Student> FromBinFile(BinaryReader reader)
        {
            var students = new List<Student>();
            while (true)
            {
                try
                {
                    var stud = new Student
                    {
                        Name = reader.ReadBytes(32),
                        Login = reader.ReadBytes(16),
                        Group = reader.ReadBytes(8),
                        Practice = reader.ReadBytes(8)
                    };
                    stud.Project.Repo = reader.ReadBytes(59);
                    stud.Project.Mark = reader.ReadByte();
                    stud.Mark = reader.ReadSingle();
                    students.Add(stud);
                }
                catch (EndOfStreamException)
                {
                    break;
                }
            }

            return students;
        }

        public static List<Student> FromKvFile(StreamReader reader)
        {
            var students = new List<Student>();
            while (!reader.EndOfStream)
            {
                var line = reader.ReadLine();
                var kv = line!.Split(' ');
                var key = kv[0];
                var value = string.Join(" ", kv.Skip(1));

                var matchName = Regex.Match(key, @"^\[(\d+)\]\.name:$");
                var matchLogin = Regex.Match(key, @"^\[(\d+)\]\.login:$");
                var matchGroup = Regex.Match(key, @"^\[(\d+)\]\.group:$");
                var matchPractice = Regex.Match(key, @"^\[(\d+)\]\.practice\.\[(\d+)\]:$");
                var matchProjectRepo = Regex.Match(key, @"^\[(\d+)\]\.project\.repo:$");
                var matchProjectMark = Regex.Match(key, @"^\[(\d+)\]\.project\.mark:$");
                var matchMark = Regex.Match(key, @"^\[(\d+)\]\.mark:$");

                var matches = new[]
                    { matchName, matchLogin, matchGroup, matchPractice, matchProjectRepo, matchProjectMark, matchMark };

                var wasAdded = false;
                foreach (var match in matches.Where(match => match.Success))
                {
                    var id = int.Parse(match.Groups[1].Value);
                    if (id > students.Count)
                        throw new ParseException("Unexpected student id");
                    if (id == students.Count && !wasAdded)
                    {
                        students.Add(new Student());
                        wasAdded = true;
                    }
                }
                
                if (matchName.Success)
                {
                    var id = int.Parse(matchName.Groups[1].Value);
                    var stud = students[id];
                    stud.Name = Encoding.UTF8.GetBytes(value);
                    Array.Resize(ref stud.Name, 32);
                    students[id] = stud;
                }
                else if (matchLogin.Success)
                {
                    var id = int.Parse(matchLogin.Groups[1].Value);
                    var stud = students[id];
                    stud.Login = Encoding.UTF8.GetBytes(value);
                    Array.Resize(ref stud.Login, 16);
                    students[id] = stud;
                }
                else if (matchGroup.Success)
                {
                    var id = int.Parse(matchGroup.Groups[1].Value);
                    var stud = students[id];
                    stud.Group = Encoding.UTF8.GetBytes(value);
                    Array.Resize(ref stud.Group, 8);
                    students[id] = stud;
                }
                else if (matchPractice.Success)
                {
                    var id = int.Parse(matchPractice.Groups[1].Value);
                    var practiceId = int.Parse(matchPractice.Groups[2].Value);
                    if (practiceId >= 8)
                        throw new ParseException("practiceId too large");
                    var stud = students[id];
                    stud.Practice ??= new byte[8];

                    if (byte.TryParse(value, out var parsedByte))
                        stud.Practice[practiceId] = parsedByte;
                    
                    students[id] = stud;
                }
                else if (matchProjectRepo.Success)
                {
                    var id = int.Parse(matchProjectRepo.Groups[1].Value);
                    var stud = students[id];
                    stud.Project.Repo = Encoding.UTF8.GetBytes(value);
                    Array.Resize(ref stud.Project.Repo, 59);
                    students[id] = stud;
                }
                else if (matchProjectMark.Success)
                {
                    var id = int.Parse(matchProjectMark.Groups[1].Value);
                    var stud = students[id];
                    stud.Project.Mark = byte.Parse(value);
                    students[id] = stud;
                }
                else if (matchMark.Success)
                {
                    var id = int.Parse(matchMark.Groups[1].Value);
                    var stud = students[id];
                    stud.Mark = float.Parse(value);
                    students[id] = stud;
                }
                else
                {
                    throw new ParseException($"Can not parse key \"{key}\"");
                }
            }

            return students;
        }

        public static void ToBinFile(List<Student> students, string filepath)
        {
            using var writer = new BinaryWriter(File.Open(filepath, FileMode.Create));

            foreach (var stud in students)
            {
                writer.Write(stud.Name);
                writer.Write(stud.Login);
                writer.Write(stud.Group);
                writer.Write(stud.Practice);
                writer.Write(stud.Project.Repo);
                writer.Write(stud.Project.Mark);
                writer.Write(stud.Mark);
            }
        }

        public static string StringFromBytes(byte[] bytes)
        {
            var index = Array.IndexOf(bytes, (byte)0);
            if (index < 0) index = bytes.Length;

            return Encoding.UTF8.GetString(bytes.Take(index).ToArray());
        }


        public static void ToKvFile(List<Student> students, string filepath)
        {
            using var writer = new StreamWriter(File.Open(filepath, FileMode.Create));
            
            for (var i = 0; i < students.Count; i++)
            {
                writer.WriteLine($"[{i}].name: {StringFromBytes(students[i].Name)}");
                writer.WriteLine($"[{i}].login: {StringFromBytes(students[i].Login)}");
                writer.WriteLine($"[{i}].group: {StringFromBytes(students[i].Group)}");
                for (var practice = 0; practice < students[i].Practice.Length; practice++)
                    writer.WriteLine($"[{i}].practice.[{practice}]: {students[i].Practice[practice]}");

                writer.WriteLine($"[{i}].project.repo: {StringFromBytes(students[i].Project.Repo)}");
                writer.WriteLine($"[{i}].project.mark: {students[i].Project.Mark}");
                writer.WriteLine($"[{i}].mark: {students[i].Mark:F3}");
            }
        }
}