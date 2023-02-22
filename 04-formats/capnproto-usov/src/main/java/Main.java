import org.capnproto.DecodeException;
import org.capnproto.MessageBuilder;
import org.capnproto.MessageReader;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.Arrays;

import static java.lang.System.exit;
import static java.lang.System.setOut;
import static org.capnproto.SerializePacked.readFromUnbuffered;
import static org.capnproto.SerializePacked.writeToUnbuffered;

public class Main {

    private static final int NAME_SIZE = 32;
    private static final int LOGIN_SIZE = 16;
    private static final int GROUP_SIZE = 8;
    private static final int PRACTICE_SIZE = 8;
    private static final int REPO_SIZE = 59;
    private static final int PROJ_MARK_SIZE = 1;
    private static final int MARK_SIZE = 4;

    public static class Student {
        public String name;
        public String login;
        public String group;
        public byte[] practice;
        public Project project;
        public float mark;

        public static Student parseBin(final ByteBuffer bytes) {
            Student student = new Student();
            byte[] a = bytes.array();
            int[] pointers = {0, NAME_SIZE};

            student.name = getString(a, pointers, LOGIN_SIZE);
            student.login = getString(a, pointers, GROUP_SIZE);
            student.group = getString(a, pointers, PRACTICE_SIZE);

            student.practice = Arrays.copyOfRange(a, pointers[0], pointers[0] + PRACTICE_SIZE);
            pointers[0] = pointers[1];
            pointers[1] += REPO_SIZE;

            student.project = new Project();
            student.project.repo = getString(a, pointers, PROJ_MARK_SIZE);
            student.project.mark = a[pointers[0]];
            pointers[0] = pointers[1];
            pointers[1] += MARK_SIZE;

            student.mark = bytes.getFloat(pointers[0]);
            return student;
        }

        public static Student readCapnproto(final FileChannel input, int idx) throws IOException {
            Student student = new Student();
//            System.out.println(input.position());
            MessageReader message = readFromUnbuffered(input);
//            System.out.println(input.position());
            input.position((idx + 1) * 136);

            Formats.Student.Reader studentReader = message.getRoot(Formats.Student.factory);
            student.name = studentReader.getName().toString();
            student.login = studentReader.getLogin().toString();
            student.group = studentReader.getGroup().toString();

            student.practice = new byte[8];
            for (int i = 0; i < 8; ++i) {
                student.practice[i] = studentReader.getPractice().get(i);
            }

            student.project = new Project();
            student.project.repo = studentReader.getProject().getRepo().toString();
            student.project.mark = studentReader.getProject().getMark();

            student.mark = studentReader.getMark();

            return student;
        }

        public String toString() {
            return String.format("name: %s\nlogin: %s\ngroup: %s\npractice: %s\nproject:\n%s\nmark: %f\n",
                                 name, login, group, Arrays.toString(practice), project, mark);
        }
    }

    public static class Project {
        public String repo;
        public byte mark;

        public String toString() {
            return String.format("\trepo: %s\n\tmark: %d", repo, mark);
        }
    }

    public static void main(final String[] args) {
        if (args.length != 1) {
            System.out.println("Usage: java Main filename");
            return;
        }
        final String filename = args[0];
        if (filename.endsWith(".bin")) {
            System.out.printf("Reading binary student data from %s...\n", filename);
            toCapnproto(filename.split("\\.bin")[0]);
        } else if (filename.endsWith(".capnproto")) {
            System.out.printf("Reading capnproto student data from %s...\n", filename);
            toBin(filename.split("\\.capnproto")[0]);
        } else {
            System.err.printf("Unknown format for file %s, must be either bin or capnproto\n", filename);
        }
    }

    public static void toBin(final String filename) {
        try (RandomAccessFile file = new RandomAccessFile(filename + ".capnproto", "r");
             FileChannel input = file.getChannel()
        ) {
            final String output = filename + ".bin";
            Files.write(Paths.get(output), new byte[0], StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING);
            int cnt = writeBin(input, output);
            System.out.printf("%d students read...\n", cnt);
            System.out.printf("written to %s.bin\n", filename);
        } catch (final IOException e) {
            e.printStackTrace();
            exit(1);
        }
    }

    public static int writeBin(final FileChannel input, final String filename) throws IOException {
        Student student;
        int cnt = 0;
        for (;; ++cnt) {
            try {
                student = Student.readCapnproto(input, cnt);
//                System.out.println(input.position());
            } catch (final DecodeException ex) {
                break;
            }

//            System.out.println(student);

            ByteBuffer buffer = ByteBuffer.allocate(128);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            putStringBytes(buffer, student.name, 32);
            putStringBytes(buffer, student.login, 16);
            putStringBytes(buffer, student.group, 8);
            buffer.put(student.practice, 0, 8);
            putStringBytes(buffer, student.project.repo, 59);
            buffer.put(student.project.mark);
            buffer.putFloat(student.mark);

            Files.write(Paths.get(filename), buffer.array(), StandardOpenOption.APPEND);
            buffer.clear();
        }
        return cnt;
    }


    private static void putStringBytes(ByteBuffer buffer, String string, int length) {
        StringBuilder builder = new StringBuilder(string);
        while (builder.length() < length) {
            builder.append("\0");
        }
        buffer.put(builder.toString().getBytes(), 0, length);
    }

    public static void toCapnproto(final String filename) {
        try (RandomAccessFile file = new RandomAccessFile(filename + ".bin", "r");
             FileChannel input = file.getChannel()
        ) {
            byte[] a = new byte[128];
            ByteBuffer b = ByteBuffer.wrap(a);
            b.order(ByteOrder.LITTLE_ENDIAN);
            int cnt = 0;
            final String output = filename + ".capnproto";
            Files.write(Paths.get(output), new byte[0], StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING);
            FileChannel fc = new FileOutputStream(output).getChannel();
            while (input.read(b) != -1) {
                writeCapnproto(b, fc);
                cnt += 1;
                b.clear();
            }
            System.out.printf("%d students read...\n", cnt);
            System.out.printf("written to %s.capnproto\n", filename);
            fc.close();
        } catch (final IOException e) {
            e.printStackTrace();
            exit(1);
        }
    }

    public static void writeCapnproto(final ByteBuffer bytes, final FileChannel fc) throws IOException {
        final Student student = Student.parseBin(bytes);
//        System.out.println(student);

        MessageBuilder message = new MessageBuilder();
        Formats.Student.Builder studentBuilder = message.initRoot(Formats.Student.factory);
        studentBuilder.setName(student.name);
        studentBuilder.setLogin(student.login);
        studentBuilder.setGroup(student.group);

        studentBuilder.initPractice(8);
        for (int i = 0; i < 8; ++i) {
            studentBuilder.getPractice().set(i, student.practice[i]);
        }

        studentBuilder.initProject();
        studentBuilder.getProject().setRepo(student.project.repo);
        studentBuilder.getProject().setMark(student.project.mark);

        studentBuilder.setMark(student.mark);

        writeToUnbuffered(fc, message);
    }

    private static String getString(byte[] a, int[] p, int sz) {
        String s;
        int i = p[0], prev = p[0], accum = p[1];
        for (; i < accum && a[i] != 0; i++) {}
        s = new String(a, prev, i-prev);
        p[0] = accum;
        p[1] += sz;
        return s;
    }

}
