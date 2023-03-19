package hse.kozlova.model;

import com.j256.ormlite.field.DataType;
import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;
import org.apache.commons.lang3.StringUtils;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

@DatabaseTable(tableName = "student")
public class Student {
    @DatabaseField
    private String name;
    @DatabaseField(id = true)
    private String login;
    @DatabaseField
    private String group;
    @DatabaseField(dataType = DataType.BYTE_ARRAY)
    private byte[] practice;
    @DatabaseField(dataType = DataType.SERIALIZABLE)
    private Project project;
    @DatabaseField
    private float mark;

    public Student() {
    }

    public String getName() {
        return name;
    }

    public void setName(byte[] name) {
        this.name = new String(name, StandardCharsets.UTF_8);
        this.name = StringUtils.stripEnd(this.name, "\0");
    }

    public byte[] getNameAsBytes() {
        StringBuilder stringBuilder = new StringBuilder(this.name);

        while (stringBuilder.length() < 32) {
            stringBuilder.append("\0");
        }

        return stringBuilder.toString().getBytes(StandardCharsets.UTF_8);
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getLogin() {
        return login;
    }

    public byte[] getLoginAsBytes() {
        StringBuilder stringBuilder = new StringBuilder(this.login);

        while (stringBuilder.length() < 16) {
            stringBuilder.append("\0");
        }

        return stringBuilder.toString().getBytes(StandardCharsets.US_ASCII);
    }

    public void setLogin(byte[] login) {
        this.login = new String(login, StandardCharsets.US_ASCII);
        this.login = StringUtils.stripEnd(this.login, "\0");
    }

    public void setLogin(String login) {
        this.login = login;
    }

    public String getGroup() {
        return group;
    }

    public byte[] getGroupAsBytes() {
        StringBuilder stringBuilder = new StringBuilder(this.group);

        while (stringBuilder.length() < 8) {
            stringBuilder.append("\0");
        }

        return stringBuilder.toString().getBytes(StandardCharsets.UTF_8);
    }

    public void setGroup(byte[] group) {
        this.group = new String(group, StandardCharsets.UTF_8);
        this.group = StringUtils.stripEnd(this.group, "\0");
    }

    public void setGroup(String group) {
        this.group = group;
    }

    public byte[] getPractice() {
        return practice;
    }

    public void setPractice(byte[] practice) {
        this.practice = practice;
    }

    public Project getProject() {
        return project;
    }

    public void setProject(Project project) {
        this.project = project;
    }

    public float getMark() {
        return mark;
    }

    public void setMark(float mark) {
        this.mark = mark;
    }

    @Override
    public String toString() {
        int[] practiceTmp = new int[practice.length];

        for (int i = 0; i < practice.length; ++i) {
            practiceTmp[i] = (int) practice[i] & 0xFF;
        }

        return "Student{" +
                "name='" + name + '\'' +
                ", login='" + login + '\'' +
                ", group='" + group + '\'' +
                ", practice=" + Arrays.toString(practiceTmp) +
                ", project=" + project +
                ", mark=" + mark +
                '}';
    }
}
