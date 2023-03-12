import java.io.*;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class Main {

    public static final int NAMESIZE = 32;
    public static final int LOGINSIZE = 16;
    public static final int GROUPSIZE = 8;
    public static final int PRACTICESIZE = 8;
    public static final int PROJECTREPOSIZE = 59;

    public static final int TOTALSIZE = 128;

    public static class Project {
        private String repo;
        private byte mark;
    }

    public static class Student {
        private String name;
        private String login;
        private String group;
        private byte[] practice;
        private Project project;
        private float mark;
    }

    private static String readString(ByteBuffer buffer, int size) {
        byte[] dst = new byte[size];
        buffer.get(dst);
        return new String(dst).replace("\0", "");
    }

    private static List<Student> readBin(Path path) throws IOException {
        List<Student> result = new ArrayList<>();
        FileChannel inChannel = new FileInputStream(path.toFile()).getChannel();
        ByteBuffer buffer = ByteBuffer.allocate(TOTALSIZE);
        while (inChannel.read(buffer) > 0) {
            buffer.flip();
            Student s = new Student();
            s.project = new Project();
            s.name = readString(buffer, NAMESIZE);
            s.login = readString(buffer, LOGINSIZE);
            s.group = readString(buffer, GROUPSIZE);
            s.practice = new byte[PRACTICESIZE];
            buffer.get(s.practice);
            s.project.repo = readString(buffer, PROJECTREPOSIZE);
            s.project.mark = buffer.get();
            s.mark = buffer.getFloat();
            result.add(s);
            buffer.clear();
        }
        return result;
    }

    private static List<Student> readProtobuf(Path path) {
        List<Student> result = new ArrayList<>();
        File inFile = path.toFile();
        FileReader fileReader = null;
        try {
            fileReader = new FileReader(inFile);
            char[] studBuf = new char[format04.PROTOSIZE];
            while (fileReader.read(studBuf) > 0) {
                Student s = new Student();
                s.project = new Project();
                format04.Student formatStudent = new format04.Student(studBuf);
                s.name = formatStudent.name;
                s.login = formatStudent.login;
                s.group = formatStudent.group;
                s.practice = formatStudent.practice;
                s.project.repo = formatStudent.project.repo;
                s.project.mark = formatStudent.project.mark;
                s.mark = formatStudent.mark;
                result.add(s);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally{
            try {
                fileReader.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return result;
    }

    private static byte[] encodeString(String s, int size){
        byte[] result = new byte[size];
        byte[] stringBytes = s.getBytes();
        System.arraycopy(stringBytes, 0, result, 0, stringBytes.length);
        return result;
    }

    private static byte[] encodeStudent(Student student){
        ByteBuffer byteBuffer = ByteBuffer.allocate(TOTALSIZE);
        byteBuffer.put(encodeString(student.name, NAMESIZE));
        byteBuffer.put(encodeString(student.login, LOGINSIZE));
        byteBuffer.put(encodeString(student.group, GROUPSIZE));
        byteBuffer.put(student.practice);
        byteBuffer.put(encodeString(student.project.repo, PROJECTREPOSIZE));
        byteBuffer.put(student.project.mark);
        byteBuffer.putFloat(student.mark);
        byteBuffer.flip();
        return byteBuffer.array();
    }

    private static void writeBin(List<Student> students, Path path) {
        OutputStream os = null;
        try {
            os = new FileOutputStream(path.toFile());
            int offset = 0;
            for (Student s: students) {
                os.write(encodeStudent(s), offset, TOTALSIZE);
                offset += TOTALSIZE;
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                os.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private static void writeProtobuf(List<Student> students, Path path) {
        File outFile = path.toFile();
        FileWriter fileWriter = null;
        try {
            fileWriter = new FileWriter(outFile);
            FileWriter finalFileWriter = fileWriter;
            students.forEach(student -> {
                try {
                    finalFileWriter.write(format04.Student.of(student).toString());
                } catch (IOException e) {
                    throw new RuntimeException(e);
                }
            });
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                fileWriter.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void main(String[] args) {
        if (args.length != 1) {
            System.out.println("Please provide the file name.");
            return;
        }
        String fileName = args[0];
        Path inPath = Path.of(fileName);
        if (inPath.endsWith(".protobuf")) {
            List<Student> students = readProtobuf(inPath);
            Path outPath = Path.of(fileName.replace(".protobuf", ".bin"));
            writeBin(students, outPath);
        } else if (inPath.endsWith(".bin")) {
            List<Student> students;
            try {
                students = readBin(inPath);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
            Path outPath = Path.of(fileName.replace(".bin", ".protobuf"));
            writeProtobuf(students, outPath);
        } else {
            System.out.println("Invalid file extension (not .bin/.protobuf).");
        }
    }
}
