package hse.kozlova;

import hse.kozlova.io.StudentIOManager;
import org.apache.commons.lang3.StringUtils;

import java.io.IOException;
import java.sql.SQLException;

public class Main {

    public static void main(String[] args) throws IOException, SQLException {
        if (args.length != 1) {
            throw new RuntimeException("Invalid number of arguments!");
        }

        var arg = args[0];

        if (arg.endsWith(".bin")) {
            var result = StudentIOManager.readStudentsFromBinaryFile(arg);
            StudentIOManager.writeStudentsToDatabaseFile(result, StringUtils.removeEnd(arg, ".bin") + ".sqlite");
        } else if (arg.endsWith(".sqlite")) {
            var result = StudentIOManager.readStudentsFromDatabaseFile(arg);
            StudentIOManager.writeStudentsToBinaryFile(result, StringUtils.removeEnd(arg, ".sqlite") + ".bin");
        } else {
            throw new RuntimeException("Unknown format of input filename!");
        }
    }
}