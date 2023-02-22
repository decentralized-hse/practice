using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;
using System.Runtime.Serialization;
using ProtoBuf;

[ProtoContract]
class Person {
    [ProtoMember(1)]
    public string Name {get;set;}
    [ProtoMember(2)]
    public string Login {get; set;}
    [ProtoMember(3)]
    public string Group {get; set;}
    [ProtoMember(4)]
    public byte[] Practice {get; set;}

    [ProtoMember(5)]
    public string PrRepo {get; set;}
    [ProtoMember(6)]
    public byte PrMark {get; set;}

    [ProtoMember(7)]
    public float Mark {get; set;}
}

class Formats {
    static void b2p(string path) {
        var persons = new List<Person>();
        using (var reader = new BinaryReader(File.OpenRead(path + ".bin"))) {
            while (reader.BaseStream.Position != reader.BaseStream.Length) {
                var person = new Person();
                person.Name = System.Text.Encoding.UTF8.GetString(reader.ReadBytes(32), 0, 32);
                person.Login = System.Text.Encoding.ASCII.GetString(reader.ReadBytes(16), 0, 16);
                person.Group = System.Text.Encoding.UTF8.GetString(reader.ReadBytes(8), 0, 8);
                person.Practice = reader.ReadBytes(8);

                person.PrRepo = System.Text.Encoding.UTF8.GetString(reader.ReadBytes(59), 0, 59);
                person.PrMark = reader.ReadByte();

                person.Mark = reader.ReadSingle();

                persons.Add(person);
            }
        }
        
        using (var file = File.Create(path + ".protobuf")) {
            Serializer.Serialize(file, persons);
        }
    }

    static void p2b(string path) {
        List<Person> persons;
        using (var file = File.OpenRead(path + ".protobuf")) {
            persons = Serializer.Deserialize<List<Person>>(file);
        }

        using (var writer = new BinaryWriter(File.OpenWrite(path + ".bin"))) {
            foreach (var person in persons) {
                var name = new byte[32];
                Encoding.UTF8.GetBytes(person.Name).CopyTo(name, 0);
                writer.Write(name);

                var login = new byte[16];
                Encoding.ASCII.GetBytes(person.Login).CopyTo(login, 0);
                writer.Write(login);

                var group = new byte[8];
                Encoding.UTF8.GetBytes(person.Group).CopyTo(group, 0);
                writer.Write(group);

                writer.Write(person.Practice);

                var prRepo = new byte[59];
                Encoding.UTF8.GetBytes(person.PrRepo).CopyTo(prRepo, 0);
                writer.Write(prRepo);
                
                writer.Write(person.PrMark);

                writer.Write(person.Mark);
            }
        }
    }

    static int Main(string[] args)
    {
        if (args[0].EndsWith(".bin")) {
            b2p(args[0].Substring(0, args[0].Length - 4));
        } else if (args[0].EndsWith(".protobuf")) {
            p2b(args[0].Substring(0, args[0].Length - 9));
        } else {
            return -1;
        }
        return 0;
    }
}