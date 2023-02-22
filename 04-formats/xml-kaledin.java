package com.company;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import static java.lang.System.exit;

public class Main {

    private static final int NAME_SIZE = 32;
    private static final int LOGIN_SIZE = 16;
    private static final int GROUP_SIZE = 8;
    private static final int PRACTICE_SIZE = 8;
    private static final int REPO_SIZE = 59;
    private static final int PROJ_MARK_SIZE = 1;
    private static final int MARK_SIZE = 4;

    public static void main(String[] args) {
        if (args.length != 1) {
            System.out.println("Must be filename");
            return;
        }
        if (args[0].contains(".")) {
            String ext = args[0].substring(args[0].lastIndexOf('.') + 1);
            String filename = args[0].substring(0, args[0].lastIndexOf('.'));
            switch (ext) {
                case "bin":
                    writeToXml(filename);
                    break;
                case "xml":
                    writeToBin(filename);
                    break;
                default:
                    System.out.println("Wrong extension, must be .bin or .xml");
                    return;
            }
        } else {
            System.out.println("Invalid filename");
            return;
        }
    }

    private static void writeToXml(String filename) {
        try (RandomAccessFile aFile = new RandomAccessFile(filename + ".bin", "r");
             FileChannel inChannel = aFile.getChannel();
             FileWriter fw = new FileWriter(filename + ".xml")) {
            byte[] a = new byte[128];
            ByteBuffer b = ByteBuffer.wrap(a);
            b.order(ByteOrder.LITTLE_ENDIAN);
            int id = 0;
            while (inChannel.read(b) != -1) {
                writeInstToXml(id, b, fw);
                id += 1;
                b.clear();
            }
        } catch (IOException e) {
            e.printStackTrace();
            exit(1);
        }
    }
    private static String getString(byte[] a, int[] p, Charset sc) {
        String s;
        int i = p[0], prev = p[1], accum = p[2];
        for (; i < accum && a[i] != 0; i++) {}
        s = new String(a, prev, i-prev, sc);
        p[0] = accum;
        p[1] = accum;
        return s;
    }
    private static void writeInstToXml(int id, ByteBuffer bb, FileWriter in) {
        String name, login, group, practice = "", repo;
        byte[] a = bb.array();

        int[] pointers = {0, 0, NAME_SIZE}; // curIndex, prevSize, nextSize
        name = getString(a, pointers, StandardCharsets.UTF_8);

        pointers[2] += LOGIN_SIZE;
        login = getString(a,pointers, StandardCharsets.US_ASCII);

        pointers[2] += GROUP_SIZE;
        group = getString(a, pointers, StandardCharsets.UTF_8);

        pointers[2] += PRACTICE_SIZE;
        for (;pointers[0] < pointers[2]; pointers[0]++) {
            if (a[pointers[0]] > 0) {
                practice += "1";
            } else {
                practice += "0";
            }
            if (pointers[0] != pointers[2] - 1) {
                practice += ", ";
            }
        }
        pointers[1] = pointers[2];

        pointers[2] += REPO_SIZE;
        repo = getString(a,pointers, StandardCharsets.US_ASCII);

        pointers[2] += PROJ_MARK_SIZE;
        int proj_mark = a[pointers[0]];
        pointers[0] = pointers[2];
        pointers[1] = pointers[2];

        pointers[2] += MARK_SIZE;
        float mark = bb.getFloat(pointers[1]);

        String str_id = "[" + id + "].";
        try {
            in.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            in.write("<student>\n");
            in.write("<name>" + name + "</name>\n");
            in.write("<login>" + login + "</login>\n");
            in.write("<group>" + group + "</group>\n");
            in.write("<practice>" + practice + "</practice>\n");
//            in.write("<project>\n");
            in.write("<repo>" + repo + "</repo>\n");
            in.write("<proj_mark>" + proj_mark + "</proj_mark>\n");
//            in.write("</project>\n");
            in.write("<mark>" + mark + "</mark>\n");
            in.write("</student>");
        } catch (IOException e) {
            e.printStackTrace();
            exit(1);
        }
    }

    private static byte[] parseString(String in) {
        if (!in.contains("</") || in.indexOf("</") < in.indexOf(">")) {
            return new byte[]{};
        }
        String key = in.substring(in.indexOf("<") + 1, in.indexOf(">"));
        String value = in.substring(in.indexOf(">") + 1, in.indexOf("</"));

        byte[] bytes;

        switch (key) {
            case "name":
                bytes = new byte[NAME_SIZE];
                return Arrays.copyOf(value.getBytes(StandardCharsets.UTF_8), NAME_SIZE);
            case "login":
                return Arrays.copyOf(value.getBytes(StandardCharsets.US_ASCII), LOGIN_SIZE);
            case "group":
                return Arrays.copyOf(value.getBytes(StandardCharsets.UTF_8), GROUP_SIZE);
            case "practice":
                bytes = new byte[PRACTICE_SIZE];
                int j = 0;
                for (int i = 0; i < value.length(); i++) {
                    char c = value.charAt(i);
                    switch (c) {
                        case '1':
                            bytes[j] = 1;
                            j++;
                            break;
                        case '0':
                            bytes[j] = 0;
                            j++;
                            break;
                        default:
                            continue;
                    }
                }
                return bytes;
            case "repo":
                return Arrays.copyOf(value.getBytes(StandardCharsets.US_ASCII), REPO_SIZE);
            case "proj_mark":
                bytes = new byte[PROJ_MARK_SIZE];
                bytes[0] = Byte.parseByte(value);
                return bytes;
            case "mark":
                float f = Float.parseFloat(value);
                int intBits =  Float.floatToIntBits(f);
                return new byte[] {
                        (byte) (intBits), (byte) (intBits >> 8), (byte) (intBits >> 16), (byte) (intBits >> 24) };
            default:
                System.out.println("Invalid field");
                exit(1);
        }
        return new byte[1];
    }
    private static void writeToBin(String filename) {
        try (FileReader fr = new FileReader(filename + ".xml");
             BufferedReader br = new BufferedReader(fr);
             DataOutputStream dataOut = new DataOutputStream(new FileOutputStream(filename + ".bin"))) {
            int id = 0;
            String q;
            byte[] toWrite;
            while ((q = br.readLine()) != null) {
                toWrite = parseString(q);
                if (toWrite.length == 0) {
                    continue;
                }
                dataOut.write(toWrite);
            }
        } catch (IOException e) {
            e.printStackTrace();
            exit(1);
        }
    }
}

