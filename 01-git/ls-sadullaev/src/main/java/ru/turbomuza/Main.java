package ru.turbomuza;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Main {
    public static String findTargetDirectory(List<String> path, String rootBlob) throws IOException {
        String currentBlob = rootBlob;
        for (var name : path) {
            if (name.isEmpty()) {
                return currentBlob;
            }
            currentBlob = findItemPathName(currentBlob, name); // +
            System.out.println(currentBlob);
        }

        return currentBlob;
    }

    public static String findItemPathName(String currentBlob, String name) throws IOException {
        BufferedReader bufferedReader = new BufferedReader(new FileReader(currentBlob));
        String startLine;
        while ((startLine = bufferedReader.readLine()) != null) {
            var parts = startLine.split("/\t");
            if (parts.length >= 2) {
                var itemName = parts[0];
                var itemPath = parts[1];
                if (itemName.equals(name)) {
                    return itemPath;
                }
            }
        }
        throw new IOException("не нашли то что искали");
    }

    public static void printTree(String blob, int depth) throws IOException {
        BufferedReader bufferedReader = new BufferedReader(new FileReader(blob));
        String startLine;
        while ((startLine = bufferedReader.readLine()) != null) {
            System.out.println("\t".repeat(depth) + startLine);
            if (startLine.contains("/\t")) {
                String[] check = startLine.split("/\t");
                printTree(check[1], depth + 1);
            } else {
                String[] blobs = startLine.split(":\t");
                String blobsPath = blobs[0];
                if (blobs.length > 1) {
                    String checkSum = checkHash(blobsPath);
                    if (!checkSum.equals(blobsPath)) {
                        throw new IOException("хэши не совпадают");
                    }
                }
            }
        }
    }

    public static String checkHash(String path) {
        return ""; // заглушка, сказали не делать
    }

    public static void main(String[] args) {
        List<String> check = List.of(args);
        if (check.size() < 2) {
            System.err.println("Неверное использование: <arg1> <arg2>");
            System.out.println(check.size());
            for(var elem: check) {
                System.out.println(elem);
            }
            System.exit(1);
        }

        var targetPathDirectory = check.get(0);
        var hash = check.get(1);
        try {
            if (targetPathDirectory.equals("")) {
                printTree(hash, 0);
            } else {
                List<String> pathDir = new ArrayList<>(Arrays.asList(targetPathDirectory.split("/")));
                var dir = findTargetDirectory(pathDir, hash);
                printTree(dir, 0);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}
