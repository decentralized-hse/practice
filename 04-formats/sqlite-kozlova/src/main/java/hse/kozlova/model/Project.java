package hse.kozlova.model;

import org.apache.commons.lang3.StringUtils;

import java.io.Serializable;
import java.nio.charset.StandardCharsets;


public class Project implements Serializable {

    private String repo;

    private byte mark;

    public Project() {
    }

    public String getRepo() {
        return repo;
    }

    public void setRepo(byte[] repo) {
        this.repo = new String(repo, StandardCharsets.US_ASCII);
        this.repo = StringUtils.stripEnd(this.repo, "\0");
    }

    public byte[] getRepoAsBytes() {
        StringBuilder stringBuilder = new StringBuilder(this.repo);

        while (stringBuilder.length() < 59) {
            stringBuilder.append("\0");
        }

        return stringBuilder.toString().getBytes(StandardCharsets.US_ASCII);
    }

    public void setRepo(String repo) {
        this.repo = repo;
    }

    public byte getMark() {
        return mark;
    }

    public void setMark(byte mark) {
        this.mark = mark;
    }

    @Override
    public String toString() {
        return "Project{" +
                "repo='" + repo + '\'' +
                ", mark=" + ((int) mark & 0xFF) +
                '}';
    }
}
