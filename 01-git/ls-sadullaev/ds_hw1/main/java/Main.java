import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
public class Main {
    public static String directorySearch(String path, String root) throws IOException {
        File directory = new File(root);
        if (directory.isDirectory()) {
            return path;
        }
        if (path.equals(".") || path.equals("./") || path.equals("")) {
            return root;
        }
        var tokens = path.split("/");
        if (tokens.length < 2) {
            System.err.println("Нету такой директории. Отчет:\n путь: " + path + "\nкорень:" + root + "\n");
            throw new RuntimeException("Некоректный ввод");
        }
        var pathPrefix = tokens[0] + "/";
        var pathSuffix = tokens[1];
        try (BufferedReader file = new BufferedReader(new FileReader(root))) {
            String line;
            while ((line = file.readLine()) != null) {
                String[] inputLines = line.split("\t");
                if (pathPrefix.equals(inputLines[0])) {
                    return directorySearch(pathSuffix, inputLines[1]);
                }
            }
        }
        System.err.println("Префикс не найден. Отчет:\n префикс: " + pathPrefix + "\nсуф: " + pathSuffix + "\n");
        throw new RuntimeException("Неверный ввод");
    }

    public static void directoryOutput(String hash, String prefix, boolean isRoot) throws IOException {
        try (BufferedReader file = new BufferedReader(new FileReader(hash))) {
            String line;
            while ((line = file.readLine()) != null) {
                var inputLines = line.split("\t");
                if (inputLines.length < 2) {
                    return;
                }
                var objectName = inputLines[0];
                var objectHash = inputLines[1];
                if (objectName.endsWith(":")) {
                    System.out.println(prefix + objectName.substring(0, objectName.length() - 1));
                } else {
                    System.out.println(prefix + objectName);
                    if (objectName.substring(0, objectName.length() - 1).equals(".parent") && isRoot) {
                        continue;
                    }
                    directoryOutput(objectHash, prefix + objectName, false);
                }
            }
        }
    }

    public static void main(String[] args) {
        if (args.length != 2) {
            System.err.println("Использовать вот так: <arg1> <arg2>\n" +
                    "где arg1 аргументом адрес директории (pwd) и arg2 хэш файла");
            System.exit(1);
        }
        var path = args[0];
        var root = args[1];
        try {
            String hash = directorySearch(path, root);
            directoryOutput(hash, "", hash.equals(root));
        } catch (IOException e) {
            System.err.println("Произошла ошибка, попробуйте перепроверить аргументы\n" + e.getMessage());
            System.exit(1);
        }
    }
}
