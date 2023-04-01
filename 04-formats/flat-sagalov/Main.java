import com.google.flatbuffers.*;
import com.google.flatbuffers.FlatBufferBuilder;

import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;

public class Main {

    static final int SIZEOF_STUDENT = 128;
    static final int SIZEOF_STUDENT_NAME = 32;
    static final int SIZEOF_STUDENT_LOGIN = 16;
    static final int SIZEOF_STUDENT_GROUP = 8;
    static final int SIZEOF_STUDENT_PRACTICE = 8;
    static final int SIZEOF_STUDENT_PROJECT_REPO = 59;


    public static void main(String[] args) {
        if (args.length != 1) {
            System.err.println("Expected file path as a single argument.\n");
            return;
        }
        String fileName = args[0];
        Path inputFilePath = Path.of(fileName);
        if (fileName.endsWith(".bin")) {
            Path outputFilePath = Path.of(fileName.substring(0, fileName.length() - 4) + ".flat");
            convertBinToFlat(inputFilePath, outputFilePath);
        } else if (fileName.endsWith(".flat")) {
            Path outputFilePath = Path.of(fileName.substring(0, fileName.length() - 5) + ".bin");
            convertFlatToBin(inputFilePath, outputFilePath);
        } else {
            System.err.println("Expected .bin or .flat format, received: " + inputFilePath.getFileName());
        }
    }

    static void convertBinToFlat(Path binFilePath, Path flatFilePath) {
        System.out.println("Reading binary student data from " + binFilePath + "...");
        FlatBufferBuilder fbb = buildFlatFromBin(binFilePath);
        if (fbb == null) return;
        writeBytes(fbb.sizedByteArray(), flatFilePath);
    }

    static FlatBufferBuilder buildFlatFromBin(Path binFilePath) {
        try {
            // Read binary file
            byte[] fileContent = Files.readAllBytes(binFilePath);
            ByteBuffer bb = ByteBuffer.wrap(fileContent);
            bb.order(ByteOrder.LITTLE_ENDIAN);

            // Get list of students from file data
            FlatBufferBuilder builder = new FlatBufferBuilder(1024);
            StudentList.startStudentsVector(builder, fileContent.length / SIZEOF_STUDENT);
            int counter = 0;
            for (int i = 0; i < fileContent.length; i += SIZEOF_STUDENT) {
                addStudent(bb, builder);
                ++counter;
            }
            System.out.println(counter + " students read...");

            // Build list of students
            int students = builder.endVector();
            StudentList.startStudentList(builder);
            StudentList.addStudents(builder, students);
            int studentList = StudentList.endStudentList(builder);
            builder.finish(studentList);
            return builder;
        } catch (IOException e) {
            System.err.println("Failed to read from file " + binFilePath);
            return null;
        }
    }

    static void addStudent(ByteBuffer bb, FlatBufferBuilder fbb) {
        int[] name = readIntArray(bb, 32);
        int[] login = readIntArray(bb, 16);
        int[] group = readIntArray(bb, 8);
        int[] practice = readIntArray(bb, 8);
        int[] projectRepo = readIntArray(bb, 59);
        int projectMark = bb.get();
        float mark = bb.getFloat();

        Student.createStudent(fbb, name, login, group, practice, projectRepo, projectMark, mark);
    }

    static int[] readIntArray(ByteBuffer bb, int length) {
        byte[] bytes = new byte[length];
        bb.get(bytes, 0, length);
        int[] res = new int[length];
        for (int i = 0; i < length; ++i) {
            res[i] = bytes[i];
        }
        return res;
    }

    static void writeBytes(byte[] data, Path outputPath) {
        try (FileOutputStream out = new FileOutputStream(outputPath.toFile())) {
            out.write(data);
            System.out.println("written to " + outputPath);
        } catch (IOException e) {
            System.err.println("Failed to write data to " + outputPath);
        }
    }

    static void convertFlatToBin(Path flatFilePath, Path binFilePath) {
        System.out.println("Reading flatbuffers student data from " + flatFilePath + "...");
        StudentList studentList = getStudentList(flatFilePath);
        if (studentList == null) return;
        System.out.println(studentList.studentsLength() + " students read...");
        writeStudentsToBin(studentList, binFilePath);
    }

    static StudentList getStudentList(Path flatFilePath) {
        try {
            byte[] fileContent = Files.readAllBytes(flatFilePath);
            ByteBuffer bb = ByteBuffer.wrap(fileContent);
            return StudentList.getRootAsStudentList(bb);
        } catch (IOException e) {
            System.err.println("Failed to read from file " + flatFilePath);
            return null;
        }
    }

    static void writeStudentsToBin(StudentList studentList, Path binFilePath) {
        int studentCount = studentList.studentsLength();
        ByteBuffer bb = ByteBuffer.allocate(studentCount * SIZEOF_STUDENT);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        for (int i = 0; i < studentCount; ++i) {
            Student student = studentList.students(i);
            bb.put(getStudentName(student), 0, SIZEOF_STUDENT_NAME);
            bb.put(getStudentLogin(student), 0, SIZEOF_STUDENT_LOGIN);
            bb.put(getStudentGroup(student), 0, SIZEOF_STUDENT_GROUP);
            bb.put(getStudentPractice(student), 0, SIZEOF_STUDENT_PRACTICE);
            bb.put(getStudentProjectRepo(student), 0, SIZEOF_STUDENT_PROJECT_REPO);
            bb.put((byte) student.project().mark());
            bb.putFloat(student.mark());
        }
        bb.flip();
        writeBytes(bb.array(), binFilePath);
    }

    static byte[] getStudentName(Student student) {
        byte[] res = new byte[SIZEOF_STUDENT_NAME];
        for (int i = 0; i < SIZEOF_STUDENT_NAME; ++i) {
            res[i] = (byte) student.name(i);
        }
        return res;
    }

    static byte[] getStudentLogin(Student student) {
        byte[] res = new byte[SIZEOF_STUDENT_LOGIN];
        for (int i = 0; i < SIZEOF_STUDENT_LOGIN; ++i) {
            res[i] = (byte) student.login(i);
        }
        return res;
    }

    static byte[] getStudentGroup(Student student) {
        byte[] res = new byte[SIZEOF_STUDENT_GROUP];
        for (int i = 0; i < SIZEOF_STUDENT_GROUP; ++i) {
            res[i] = (byte) student.group(i);
        }
        return res;
    }

    static byte[] getStudentPractice(Student student) {
        byte[] res = new byte[SIZEOF_STUDENT_PRACTICE];
        for (int i = 0; i < SIZEOF_STUDENT_PRACTICE; ++i) {
            res[i] = (byte) student.practice(i);
        }
        return res;
    }

    static byte[] getStudentProjectRepo(Student student) {
        byte[] res = new byte[SIZEOF_STUDENT_PROJECT_REPO];
        for (int i = 0; i < SIZEOF_STUDENT_PROJECT_REPO; ++i) {
            res[i] = (byte) student.project().repo(i);
        }
        return res;
    }
}

@SuppressWarnings("unused")
public final class Project extends Struct {
    public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
    public Project __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

    public int repo(int j) { return bb.get(bb_pos + 0 + j * 1); }
    public int mark() { return bb.get(bb_pos + 59) & 0xFF; }

    public static int createProject(FlatBufferBuilder builder, int[] repo, int mark) {
        builder.prep(1, 60);
        builder.putByte((byte) mark);
        for (int _idx0 = 59; _idx0 > 0; _idx0--) {
            builder.putByte((byte) repo[_idx0-1]);
        }
        return builder.offset();
    }

    public static final class Vector extends BaseVector {
        public Project.Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

        public Project get(int j) { return get(new Project(), j); }
        public Project get(Project obj, int j) {  return obj.__assign(__element(j), bb); }
    }
}

@SuppressWarnings("unused")
public final class Student extends Struct {
    public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
    public Student __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

    public int name(int j) { return bb.get(bb_pos + 0 + j * 1); }
    public int login(int j) { return bb.get(bb_pos + 32 + j * 1); }
    public int group(int j) { return bb.get(bb_pos + 48 + j * 1); }
    public int practice(int j) { return bb.get(bb_pos + 56 + j * 1); }
    public Project project() { return project(new Project()); }
    public Project project(Project obj) { return obj.__assign(bb_pos + 64, bb); }
    public float mark() { return bb.getFloat(bb_pos + 124); }

    public static int createStudent(FlatBufferBuilder builder, int[] name, int[] login, int[] group, int[] practice, int[] project_repo, int project_mark, float mark) {
        builder.prep(4, 128);
        builder.putFloat(mark);
        builder.prep(1, 60);
        builder.putByte((byte) project_mark);
        for (int _idx0 = 59; _idx0 > 0; _idx0--) {
            builder.putByte((byte) project_repo[_idx0-1]);
        }
        for (int _idx0 = 8; _idx0 > 0; _idx0--) {
            builder.putByte((byte) practice[_idx0-1]);
        }
        for (int _idx0 = 8; _idx0 > 0; _idx0--) {
            builder.putByte((byte) group[_idx0-1]);
        }
        for (int _idx0 = 16; _idx0 > 0; _idx0--) {
            builder.putByte((byte) login[_idx0-1]);
        }
        for (int _idx0 = 32; _idx0 > 0; _idx0--) {
            builder.putByte((byte) name[_idx0-1]);
        }
        return builder.offset();
    }

    public static final class Vector extends BaseVector {
        public Student.Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

        public Student get(int j) { return get(new Student(), j); }
        public Student get(Student obj, int j) {  return obj.__assign(__element(j), bb); }
    }
}

@SuppressWarnings("unused")
public final class StudentList extends Table {
    public static void ValidateVersion() { Constants.FLATBUFFERS_23_3_3(); }
    public static StudentList getRootAsStudentList(ByteBuffer _bb) { return getRootAsStudentList(_bb, new StudentList()); }
    public static StudentList getRootAsStudentList(ByteBuffer _bb, StudentList obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
    public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
    public StudentList __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

    public Student students(int j) { return students(new Student(), j); }
    public Student students(Student obj, int j) { int o = __offset(4); return o != 0 ? obj.__assign(__vector(o) + j * 128, bb) : null; }
    public int studentsLength() { int o = __offset(4); return o != 0 ? __vector_len(o) : 0; }
    public Student.Vector studentsVector() { return studentsVector(new Student.Vector()); }
    public Student.Vector studentsVector(Student.Vector obj) { int o = __offset(4); return o != 0 ? obj.__assign(__vector(o), 128, bb) : null; }

    public static int createStudentList(FlatBufferBuilder builder,
                                        int studentsOffset) {
        builder.startTable(1);
        StudentList.addStudents(builder, studentsOffset);
        return StudentList.endStudentList(builder);
    }

    public static void startStudentList(FlatBufferBuilder builder) { builder.startTable(1); }
    public static void addStudents(FlatBufferBuilder builder, int studentsOffset) { builder.addOffset(0, studentsOffset, 0); }
    public static void startStudentsVector(FlatBufferBuilder builder, int numElems) { builder.startVector(128, numElems, 4); }
    public static int endStudentList(FlatBufferBuilder builder) {
        int o = builder.endTable();
        return o;
    }
    public static void finishStudentListBuffer(FlatBufferBuilder builder, int offset) { builder.finish(offset); }
    public static void finishSizePrefixedStudentListBuffer(FlatBufferBuilder builder, int offset) { builder.finishSizePrefixed(offset); }

    public static final class Vector extends BaseVector {
        public StudentList.Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

        public StudentList get(int j) { return get(new StudentList(), j); }
        public StudentList get(StudentList obj, int j) {  return obj.__assign(__indirect(__element(j), bb), bb); }
    }
}
