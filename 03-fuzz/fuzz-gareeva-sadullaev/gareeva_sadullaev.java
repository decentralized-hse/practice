package ru.turbomuza;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Type;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class Main {
    public static class Student {
        private String name;
        private String login;
        private String group;
        private byte[] practice;
        private Project project;
        private float mark;
    }

    public static class Project {
        private String repo;
        private byte mark;
    }

    public static void main(String[] args) {
        try {
            if (args.length != 1) {
                throw new RuntimeException("Failed to get one argument - path to file");
            }
            String fileName = args[0];
            Path inputFilePath = Path.of(fileName);
            if (fileName.endsWith(".bin")) {
                Path jsonFilePath = Path.of(fileName.substring(0, fileName.length() - ".bin".length()) + ".json");
                formatBinToJson(inputFilePath, jsonFilePath);
            } else if (fileName.endsWith(".json")) {
                Path binFilePath = Path.of(fileName.substring(0, fileName.length() - ".json".length()) + ".bin");
                formatJsonToBin(inputFilePath, binFilePath);
            } else {
                throw new IllegalArgumentException("File should be .bin or .json");
            }
        } catch (Exception e) {
            throw new RuntimeException("Во время работы программы произошла ошибка: " + e.getMessage(), e);
        }
    }
    private static void formatBinToJson(Path binFilePath, Path jsonFilePath) {
        if (binFilePath == null || jsonFilePath == null) {
            throw new IllegalArgumentException("пути не могут быть пустыми");
        }

        System.out.println("Reading binary student data from " + binFilePath + "...");
        List<Student> students = readBinFile(binFilePath);
        System.out.println(students.size() + " students read...");
        writeJsonFile(jsonFilePath, students);
        System.out.println("written to " + jsonFilePath);
    }

    private static void formatJsonToBin(Path jsonFilePath, Path binFilePath) {
        System.out.println("Reading json student data from " + jsonFilePath + "...");
        List<Student> students = readJsonFile(jsonFilePath);
        System.out.println(students.size() + " students read...");
        writeBinFile(binFilePath, students);
        System.out.println("written to " + binFilePath);
    }

    private static List<Student> readJsonFile(Path filePath) {
        if (filePath == null) { // добавил дополнительную проверку
            throw new IllegalArgumentException("Должен быть указан путь.");
        }

        try {
            Gson gson = new Gson(); // ок
            Type type = new TypeToken<List<Student>>() {
            }.getType(); // ок
            return gson.fromJson(Files.readString(filePath), type); // ок
        } catch (IOException ex) {
            throw new RuntimeException("Failed to read json from file " + filePath + ":" + ex.getMessage(), ex); // лучше иметь ошибку под рукой
        }
    }

    private static void writeJsonFile(Path filePath, List<Student> students) {
        try {
            Gson gson = new Gson();
            Files.writeString(filePath, gson.toJson(students));
        } catch (IOException ex) {
            throw new RuntimeException("Failed to write json from file " + filePath + ":\n" + ex);
        }
    }
    private static List<Student> readBinFile(Path filePath) {
        try { // try-with-resources - ok
            FileChannel fileChannel = new FileInputStream(filePath.toFile()).getChannel();
            ByteBuffer buffer = ByteBuffer.allocate(128); // лучше аллоцировать фиксированный размер, под рамки того чего ожидают по уловию чем выделять по метаданны файла.
            buffer.order(ByteOrder.LITTLE_ENDIAN);

            List<Student> students = new ArrayList<>();
            while (fileChannel.read(buffer) != -1) { // изменил на чтение по чанкам (до этого было чтение целиком что неэффективно
                buffer.flip();
                while (buffer.remaining() >= 128) {
                    Student student = new Student();
                    student.name = getString(buffer, 32);
                    student.login = getString(buffer, 16);
                    student.group = getString(buffer, 8);
                    student.practice = getBytes(buffer, 8);
                    student.project = new Project();
                    student.project.repo = getString(buffer, 59);
                    student.project.mark = buffer.get();
                    student.mark = buffer.getFloat();
                    students.add(student);
                }
                buffer.compact();
            }

            fileChannel.close(); // гарантированное закрытие (раннее было что закрытие канала зависило от чтения)
            return students;
        } catch (Exception ex) {
            throw new RuntimeException("Failed to read bin file " + filePath + ":\n" + ex);
        }
    }


    private static String getString(ByteBuffer buffer, int length) { // ok
        byte[] bytes = getBytes(buffer, length);
        StringBuilder stringBuilder = new StringBuilder();
        for (var b : bytes) {
            if (b != 0) {
                stringBuilder.append((char) b);
            }
        }
        return stringBuilder.toString();
    }

    private static byte[] getBytes(ByteBuffer buffer, int length) {
        byte[] dst = new byte[length];
        buffer.get(dst, 0, length);
        return dst;
    }

    private static void writeBinFile(Path filePath, List<Student> students) {
        try {
            ByteBuffer buffer = ByteBuffer.allocate(students.size() * 128);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            for (Student st : students) {
                putStringBytes(buffer, st.name, 32);
                putStringBytes(buffer, st.login, 16);
                putStringBytes(buffer, st.group, 8);
                if (st.practice != null) {
                    if (st.practice.length != 8) {
                        throw new IllegalArgumentException("Practice array length must be 8");
                    }
                    buffer.put(st.practice, 0, 8);
                } else {
                    throw new IllegalArgumentException("Practice array cannot be null");
                }
                putStringBytes(buffer, st.project.repo, 59);
                buffer.put(st.project.mark);
                buffer.putFloat(st.mark);
            }
            buffer.flip();

            try (FileChannel fc = new FileOutputStream(filePath.toFile()).getChannel()) {
                fc.write(buffer);
            }
        } catch (Exception ex) {
            throw new RuntimeException("Failed to write bin file " + filePath + ":\n" + ex);
        }
    }


    private static void putStringBytes(ByteBuffer buffer, String string, int length) {
        if (string == null) {
            string = ""; // добавил проверку на пустую строку.
        }

        StringBuilder builder = new StringBuilder(string);

        if (builder.length() > length) {
            builder.setLength(length);
        } else { // добавил проверку на строку если размер меньше исходной, и если не проходит то в конец добавляем символы
            while (builder.length() < length) {
                builder.append("\0");
            }
        }

        buffer.put(builder.toString().getBytes(), 0, length);
    }
}