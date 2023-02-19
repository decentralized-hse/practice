package hse.kozlova.io;

import com.j256.ormlite.dao.DaoManager;
import com.j256.ormlite.jdbc.JdbcPooledConnectionSource;
import com.j256.ormlite.table.TableUtils;
import hse.kozlova.model.Project;
import hse.kozlova.model.Student;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class StudentIOManager {

    private static final String SQLLITE_JDBC = "jdbc:sqlite:";

    public static List<Student> readStudentsFromBinaryFile(String inputFilename) throws IOException {
        List<Student> result = new ArrayList<>();

        try (RandomAccessFile aFile = new RandomAccessFile(inputFilename, "r");
             FileChannel inChannel = aFile.getChannel()) {

            long fileSize = inChannel.size();

            ByteBuffer buffer = ByteBuffer.allocate((int) fileSize);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            inChannel.read(buffer);
            buffer.flip();

            for (int i = 0; i < fileSize; i += 128) {
                Student student = new Student();

                byte[] name = new byte[32];
                buffer.get(name, 0, 32);
                student.setName(name);

                byte[] login = new byte[16];
                buffer.get(login, 0, 16);
                student.setLogin(login);

                byte[] group = new byte[8];
                buffer.get(group, 0, 8);
                student.setGroup(group);

                byte[] practice = new byte[8];
                buffer.get(practice, 0, 8);
                student.setPractice(practice);

                Project project = new Project();

                byte[] repo = new byte[59];
                buffer.get(repo, 0, 59);
                project.setRepo(repo);
                project.setMark(buffer.get());
                student.setProject(project);

                student.setMark(buffer.getFloat());

                result.add(student);
            }
        }

        return result;
    }

    public static void writeStudentsToBinaryFile(List<Student> students, String outputFilename) throws IOException {
        try (RandomAccessFile aFile = new RandomAccessFile(outputFilename, "rw");
             FileChannel outChannel = aFile.getChannel()) {
            ByteBuffer byteBuffer = ByteBuffer.allocate(students.size() * 128);
            byteBuffer.order(ByteOrder.LITTLE_ENDIAN);

            for (Student st : students) {
                byteBuffer.put(st.getNameAsBytes(), 0, 32);
                byteBuffer.put(st.getLoginAsBytes(), 0, 16);
                byteBuffer.put(st.getGroupAsBytes(), 0, 8);
                byteBuffer.put(st.getPractice(), 0, 8);

                Project p = st.getProject();
                byteBuffer.put(p.getRepoAsBytes(), 0, 59);
                byteBuffer.put(p.getMark());

                byteBuffer.putFloat(st.getMark());
            }

            byteBuffer.flip();
            outChannel.write(byteBuffer);
        }
    }

    public static List<Student> readStudentsFromDatabaseFile(String databaseName) throws SQLException, IOException {
        List<Student> result;

        try (var connectionSource = new JdbcPooledConnectionSource(SQLLITE_JDBC + databaseName)) {
            connectionSource.setCheckConnectionsEveryMillis(60 * 1000);
            connectionSource.setTestBeforeGet(true);

            TableUtils.createTableIfNotExists(connectionSource, Student.class);
            var dao = DaoManager.createDao(connectionSource, Student.class);
            result = Optional.ofNullable(dao.queryForAll()).orElse(new ArrayList<>());
        }

        return result;
    }

    public static void writeStudentsToDatabaseFile(List<Student> students, String databaseName) throws SQLException, IOException {
        try (var connectionSource = new JdbcPooledConnectionSource(SQLLITE_JDBC + databaseName)) {
            connectionSource.setCheckConnectionsEveryMillis(60 * 1000);
            connectionSource.setTestBeforeGet(true);

            TableUtils.createTableIfNotExists(connectionSource, Student.class);
            var dao = DaoManager.createDao(connectionSource, Student.class);
            for (Student st : students) {
                dao.create(st);
            }
        }
    }
}
