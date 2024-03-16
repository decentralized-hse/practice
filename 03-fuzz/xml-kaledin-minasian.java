import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.HashMap;

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
            System.out.println("Specify filename as CLI argument.");
            exit(1);
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
                    System.out.println("Invalid file extension: use '.bin' for raw-byte data and '.xml' for xml");
                    exit(1);
                    break;
            }
        } else {
            System.out.println("Invalid file extension: use '.bin' for raw-byte data and '.xml' for xml");
            exit(1);
        }
    }

    private static void writeToXml(String filename) {
        try (InputStream binIn = new FileInputStream(new File(filename + ".bin"));
             BufferedWriter xmlOut = new BufferedWriter(new FileWriter(new File(filename + ".xml")))) {
            binToXml(binIn, xmlOut);
            xmlOut.close();
            binIn.close();
            exit(0);
        } catch (IOException e) {
            System.out.println("IO error");
            exit(1);
        } catch (BinaryParsingException e) {
            System.out.println("Malformed input: " + e);
            exit(1);
        }
    }

    // @author: levon12341
    // Separate parsing from writing to file.
    // Useful for fuzzing.
    public static void binToXml(InputStream binIn, BufferedWriter xmlOut) throws IOException, BinaryParsingException {
        byte[] a = new byte[128];
        int id = 0;
        int read_size; 
        while ((read_size = binIn.read(a)) != -1) {
            if (read_size > 0 && read_size % 128 == 0) {
                writeInstToXml(id, a, xmlOut);
                id += 1;
            } else {
                throw new BinaryParsingException("Expected read of size 128, got: " + read_size);
            }
        }
        if (id == 0) {
            throw new BinaryParsingException("Unexpected EOF");
        }
    }

    private static String getCharsetString(ByteBuffer bb, Charset cs) throws BinaryParsingException {
        CharsetDecoder decoder = cs.newDecoder();
        String res;
        try {
            res = decoder.decode(bb).toString();
        } catch (CharacterCodingException e) {
            throw new BinaryParsingException("Invalid UTF-8 string encountered: " + e);
        }
        return res;
    }
   
    // Use BufferedWriter instead of FileWriter (for fuzzing)
    private static void writeInstToXml(int id, byte[] bts, BufferedWriter in) throws IOException, BinaryParsingException {
        if (bts.length != 128) {
            throw new BinaryParsingException("Unknown error");
        }

        String name, login, group, practice, repo;
        ByteBuffer bb;
        int pos = 0;

        bb = ByteBuffer.wrap(bts, pos, NAME_SIZE);
        name = getCharsetString(bb, StandardCharsets.UTF_8);
        pos += NAME_SIZE;

        bb = ByteBuffer.wrap(bts, pos, LOGIN_SIZE);
        login = getCharsetString(bb, StandardCharsets.US_ASCII);
        pos += LOGIN_SIZE;

        bb = ByteBuffer.wrap(bts, pos, GROUP_SIZE);
        group = getCharsetString(bb, StandardCharsets.UTF_8);
        pos += GROUP_SIZE;

        practice = "";
        for (int i = 0; i < PRACTICE_SIZE; ++i) {
            switch (bts[pos]) {
                case 0:
                    practice += "0";
                    break;
                case 1:
                    practice += "1";
                    break;
                default:
                    throw new BinaryParsingException("Expected 0 or 1, got: " + bts[pos]);
            }
            if (i + 1 < PRACTICE_SIZE) {
                practice += ", ";
            }
            ++pos;
        }

        bb = ByteBuffer.wrap(bts, pos, REPO_SIZE);
        repo = getCharsetString(bb, StandardCharsets.US_ASCII);
        pos += REPO_SIZE;

        int proj_mark = bts[pos];
        pos += PROJ_MARK_SIZE;

        bb = ByteBuffer.wrap(bts, pos, MARK_SIZE).order(ByteOrder.LITTLE_ENDIAN);
        float mark = bb.getFloat();
        if (Float.isNaN(mark)) {
            throw new BinaryParsingException("Encountered NaN in mark");
        }
        pos += MARK_SIZE;

//        in.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        in.write("<student>\n");
        in.write("<name>" + name + "</name>\n");
        in.write("<login>" + login + "</login>\n");
        in.write("<group>" + group + "</group>\n");
        in.write("<practice>" + practice + "</practice>\n");
//        in.write("<project>\n");
        in.write("<repo>" + repo + "</repo>\n");
        in.write("<proj_mark>" + proj_mark + "</proj_mark>\n");
//        in.write("</project>\n");
        in.write("<mark>" + mark + "</mark>\n");
        in.write("</student>\n");
    }

    private static boolean verifyViewedTagsOnFinish(Map<String, byte[]> viewedTags) {
        Set<String> expected = new HashSet<>();
        expected.add("student");
        expected.add("name");
        expected.add("login");
        expected.add("group");
        expected.add("practice");
        expected.add("repo");
        expected.add("proj_mark");
        expected.add("mark");
        expected.add("/student");
        return expected.equals(viewedTags.keySet());
    }

    private static void parseString(String in, Map<String, byte[]> viewedTags) throws XmlParsingException {
        // 1. remove all leading and trailing whitespace
        in = in.trim();
        if (in.isEmpty()) {
            return;
        }

        // 2. verify pattern
        Pattern rootOpenPattern = Pattern.compile("^<student>$");
        Matcher rootOpenMatcher = rootOpenPattern.matcher(in);
        if (rootOpenMatcher.matches()) {
            if (!viewedTags.isEmpty()) {
                throw new XmlParsingException("Unexpected root tag <student>");
            }
            viewedTags.put("student", new byte[]{});
            return;
        } else if (viewedTags.isEmpty()) {
            throw new XmlParsingException("Expected root tag <student>");
        }
        
        Pattern rootClosePattern = Pattern.compile("^</student>$");
        Matcher rootCloseMatcher = rootClosePattern.matcher(in);
        if (rootCloseMatcher.matches()) {
            viewedTags.put("/student", new byte[]{});
            if (!verifyViewedTagsOnFinish(viewedTags)) {
                throw new XmlParsingException("Unexpected root tag </student>");
            }
            return;
        }

        Pattern xmlTagPattern = Pattern.compile("^<(\\w+)>(.+)</(\\w+)>$");
        Matcher matcher = xmlTagPattern.matcher(in);
        if (!matcher.matches()) {
            throw new XmlParsingException("Unknown pattern for string: '" + in + "'");
        }

        String tagOpening = matcher.group(1);
        String tagValue = matcher.group(2);
        String tagClosing = matcher.group(3);
        if (!tagOpening.equals(tagClosing)) {
            throw new XmlParsingException("Closing tag '" + tagClosing + "' does not match opening '" + tagOpening + "'");
        }

        // 3. process tag

        String key = tagOpening;
        String value = tagValue;

        if (viewedTags.containsKey(key)) {
            throw new XmlParsingException("Key '" + key + "' was already parsed");
        }

        byte[] bytes;

        switch (key) {
            case "name":
                bytes = new byte[NAME_SIZE];
                viewedTags.put("name", Arrays.copyOf(value.getBytes(StandardCharsets.UTF_8), NAME_SIZE));
                return;
            case "login":
                viewedTags.put("login", Arrays.copyOf(value.getBytes(StandardCharsets.US_ASCII), LOGIN_SIZE));
                return;
            case "group":
                viewedTags.put("group", Arrays.copyOf(value.getBytes(StandardCharsets.UTF_8), GROUP_SIZE));
                return;
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
                viewedTags.put("practice", bytes);
                return;
            case "repo":
                viewedTags.put("repo", Arrays.copyOf(value.getBytes(StandardCharsets.US_ASCII), REPO_SIZE));
                return;
            case "proj_mark":
                bytes = new byte[PROJ_MARK_SIZE];
                bytes[0] = Byte.parseByte(value);
                viewedTags.put("proj_mark", bytes);
                return;
            case "mark":
                float f = Float.parseFloat(value);
                int intBits =  Float.floatToIntBits(f);
                bytes = new byte[] {
                        (byte) (intBits), (byte) (intBits >> 8), (byte) (intBits >> 16), (byte) (intBits >> 24) };
                viewedTags.put("mark", bytes);
                return;
            default:
                throw new XmlParsingException("Unknown key: '" + key + "'");
        }
    }

    private static void writeToBin(String filename) {
        try (FileReader fr = new FileReader(filename + ".xml");
             BufferedReader xmlIn = new BufferedReader(fr);
             OutputStream binOut = new FileOutputStream(filename + ".bin")) {
            xmlToBin(xmlIn, binOut);
            xmlIn.close();
            binOut.close();
            exit(0);
        } catch (IOException e) {
            System.out.println("IO error");
            exit(1);
        } catch (XmlParsingException e) {
            System.out.println("Malformed input: " + e);
            exit(1);
        }
    }

    // @author: levon12341
    // Separate parsing from writing to file. 
    // Makes fuzzing easier.
    public static void xmlToBin(BufferedReader xmlIn, OutputStream binOut) throws IOException, XmlParsingException {
        Map<String, byte[]> viewedTags = new HashMap<>();
        String line;
        while ((line = xmlIn.readLine()) != null) {
            parseString(line, viewedTags);
        }

        if (!verifyViewedTagsOnFinish(viewedTags)) {
            throw new XmlParsingException("Unknown error");
        }

        String[] keys = new String[] {
            "name", 
            "login",
            "group",
            "practice",
            "repo",
            "proj_mark",
            "mark"
        };

        for (String key : keys) {
            binOut.write(viewedTags.get(key));
        }
    }
}


class XmlParsingException extends Exception {
    XmlParsingException(String message) {
        super(message);
    }
}

class BinaryParsingException extends Exception {
    BinaryParsingException(String message) {
        super(message);
    }
}
