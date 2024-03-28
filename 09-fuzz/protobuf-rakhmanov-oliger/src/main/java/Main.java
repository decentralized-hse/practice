import static java.lang.System.exit;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public final class Main {
  private Main() {}
  public static void
  registerAllExtensions(com.google.protobuf.ExtensionRegistryLite registry) {}

  public static void
  registerAllExtensions(com.google.protobuf.ExtensionRegistry registry) {
    registerAllExtensions((com.google.protobuf.ExtensionRegistryLite)registry);
  }
  public interface ProjectOrBuilder extends
      // @@protoc_insertion_point(interface_extends:Project)
      com.google.protobuf.MessageOrBuilder {

    /**
     * <code>string repo = 1;</code>
     * @return The repo.
     */
    java.lang.String getRepo();
    /**
     * <code>string repo = 1;</code>
     * @return The bytes for repo.
     */
    com.google.protobuf.ByteString getRepoBytes();

    /**
     * <code>uint32 mark = 2;</code>
     * @return The mark.
     */
    int getMark();
  }
  /**
   * Protobuf type {@code Project}
   */
  public static final class Project
      extends com.google.protobuf.GeneratedMessageV3 implements
          // @@protoc_insertion_point(message_implements:Project)
          ProjectOrBuilder {
    private static final long serialVersionUID = 0L;
    // Use Project.newBuilder() to construct.
    private Project(com.google.protobuf.GeneratedMessageV3.Builder<?> builder) {
      super(builder);
    }
    private Project() { repo_ = ""; }

    @java.lang.Override
    @SuppressWarnings({"unused"})
    protected java.lang.Object newInstance(UnusedPrivateParameter unused) {
      return new Project();
    }

    public static final com.google.protobuf.Descriptors.Descriptor
    getDescriptor() {
      return Main.internal_static_Project_descriptor;
    }

    @java.lang.Override
    protected com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
    internalGetFieldAccessorTable() {
      return Main.internal_static_Project_fieldAccessorTable
          .ensureFieldAccessorsInitialized(Main.Project.class,
                                           Main.Project.Builder.class);
    }

    public static final int REPO_FIELD_NUMBER = 1;
    @SuppressWarnings("serial") private volatile java.lang.Object repo_ = "";
    /**
     * <code>string repo = 1;</code>
     * @return The repo.
     */
    @java.lang.Override
    public java.lang.String getRepo() {
      java.lang.Object ref = repo_;
      if (ref instanceof java.lang.String) {
        return (java.lang.String)ref;
      } else {
        com.google.protobuf.ByteString bs = (com.google.protobuf.ByteString)ref;
        java.lang.String s = bs.toStringUtf8();
        repo_ = s;
        return s;
      }
    }
    /**
     * <code>string repo = 1;</code>
     * @return The bytes for repo.
     */
    @java.lang.Override
    public com.google.protobuf.ByteString getRepoBytes() {
      java.lang.Object ref = repo_;
      if (ref instanceof java.lang.String) {
        com.google.protobuf.ByteString b =
            com.google.protobuf.ByteString.copyFromUtf8((java.lang.String)ref);
        repo_ = b;
        return b;
      } else {
        return (com.google.protobuf.ByteString)ref;
      }
    }

    public static final int MARK_FIELD_NUMBER = 2;
    private int mark_ = 0;
    /**
     * <code>uint32 mark = 2;</code>
     * @return The mark.
     */
    @java.lang.Override
    public int getMark() {
      return mark_;
    }

    private byte memoizedIsInitialized = -1;
    @java.lang.Override
    public final boolean isInitialized() {
      byte isInitialized = memoizedIsInitialized;
      if (isInitialized == 1)
        return true;
      if (isInitialized == 0)
        return false;

      memoizedIsInitialized = 1;
      return true;
    }

    @java.lang.Override
    public void writeTo(com.google.protobuf.CodedOutputStream output)
        throws java.io.IOException {
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(repo_)) {
        com.google.protobuf.GeneratedMessageV3.writeString(output, 1, repo_);
      }
      if (mark_ != 0) {
        output.writeUInt32(2, mark_);
      }
      getUnknownFields().writeTo(output);
    }

    @java.lang.Override
    public int getSerializedSize() {
      int size = memoizedSize;
      if (size != -1)
        return size;

      size = 0;
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(repo_)) {
        size +=
            com.google.protobuf.GeneratedMessageV3.computeStringSize(1, repo_);
      }
      if (mark_ != 0) {
        size +=
            com.google.protobuf.CodedOutputStream.computeUInt32Size(2, mark_);
      }
      size += getUnknownFields().getSerializedSize();
      memoizedSize = size;
      return size;
    }

    @java.lang.Override
    public boolean equals(final java.lang.Object obj) {
      if (obj == this) {
        return true;
      }
      if (!(obj instanceof Main.Project)) {
        return super.equals(obj);
      }
      Main.Project other = (Main.Project)obj;

      if (!getRepo().equals(other.getRepo()))
        return false;
      if (getMark() != other.getMark())
        return false;
      if (!getUnknownFields().equals(other.getUnknownFields()))
        return false;
      return true;
    }

    @java.lang.Override
    public int hashCode() {
      if (memoizedHashCode != 0) {
        return memoizedHashCode;
      }
      int hash = 41;
      hash = (19 * hash) + getDescriptor().hashCode();
      hash = (37 * hash) + REPO_FIELD_NUMBER;
      hash = (53 * hash) + getRepo().hashCode();
      hash = (37 * hash) + MARK_FIELD_NUMBER;
      hash = (53 * hash) + getMark();
      hash = (29 * hash) + getUnknownFields().hashCode();
      memoizedHashCode = hash;
      return hash;
    }

    public static Main.Project parseFrom(java.nio.ByteBuffer data)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data);
    }
    public static Main.Project
    parseFrom(java.nio.ByteBuffer data,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data, extensionRegistry);
    }
    public static Main.Project parseFrom(com.google.protobuf.ByteString data)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data);
    }
    public static Main.Project
    parseFrom(com.google.protobuf.ByteString data,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data, extensionRegistry);
    }
    public static Main.Project parseFrom(byte[] data)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data);
    }
    public static Main.Project
    parseFrom(byte[] data,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data, extensionRegistry);
    }
    public static Main.Project parseFrom(java.io.InputStream input)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(PARSER,
                                                                         input);
    }
    public static Main.Project
    parseFrom(java.io.InputStream input,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(
          PARSER, input, extensionRegistry);
    }
    public static Main.Project parseDelimitedFrom(java.io.InputStream input)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3
          .parseDelimitedWithIOException(PARSER, input);
    }
    public static Main.Project parseDelimitedFrom(
        java.io.InputStream input,
        com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3
          .parseDelimitedWithIOException(PARSER, input, extensionRegistry);
    }
    public static Main.Project
    parseFrom(com.google.protobuf.CodedInputStream input)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(PARSER,
                                                                         input);
    }
    public static Main.Project
    parseFrom(com.google.protobuf.CodedInputStream input,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(
          PARSER, input, extensionRegistry);
    }

    @java.lang.Override
    public Builder newBuilderForType() {
      return newBuilder();
    }
    public static Builder newBuilder() { return DEFAULT_INSTANCE.toBuilder(); }
    public static Builder newBuilder(Main.Project prototype) {
      return DEFAULT_INSTANCE.toBuilder().mergeFrom(prototype);
    }
    @java.lang.Override
    public Builder toBuilder() {
      return this == DEFAULT_INSTANCE ? new Builder()
                                      : new Builder().mergeFrom(this);
    }

    @java.lang.Override
    protected Builder newBuilderForType(
        com.google.protobuf.GeneratedMessageV3.BuilderParent parent) {
      Builder builder = new Builder(parent);
      return builder;
    }
    /**
     * Protobuf type {@code Project}
     */
    public static final class Builder
        extends com.google.protobuf.GeneratedMessageV3.Builder<Builder>
        implements
            // @@protoc_insertion_point(builder_implements:Project)
            Main.ProjectOrBuilder {
      public static final com.google.protobuf.Descriptors.Descriptor
      getDescriptor() {
        return Main.internal_static_Project_descriptor;
      }

      @java.lang.Override
      protected com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internalGetFieldAccessorTable() {
        return Main.internal_static_Project_fieldAccessorTable
            .ensureFieldAccessorsInitialized(Main.Project.class,
                                             Main.Project.Builder.class);
      }

      // Construct using Main.Project.newBuilder()
      private Builder() {}

      private Builder(
          com.google.protobuf.GeneratedMessageV3.BuilderParent parent) {
        super(parent);
      }
      @java.lang.Override
      public Builder clear() {
        super.clear();
        bitField0_ = 0;
        repo_ = "";
        mark_ = 0;
        return this;
      }

      @java.lang.Override
      public com.google.protobuf.Descriptors.Descriptor getDescriptorForType() {
        return Main.internal_static_Project_descriptor;
      }

      @java.lang.Override
      public Main.Project getDefaultInstanceForType() {
        return Main.Project.getDefaultInstance();
      }

      @java.lang.Override
      public Main.Project build() {
        Main.Project result = buildPartial();
        if (!result.isInitialized()) {
          throw newUninitializedMessageException(result);
        }
        return result;
      }

      @java.lang.Override
      public Main.Project buildPartial() {
        Main.Project result = new Main.Project(this);
        if (bitField0_ != 0) {
          buildPartial0(result);
        }
        onBuilt();
        return result;
      }

      private void buildPartial0(Main.Project result) {
        int from_bitField0_ = bitField0_;
        if (((from_bitField0_ & 0x00000001) != 0)) {
          result.repo_ = repo_;
        }
        if (((from_bitField0_ & 0x00000002) != 0)) {
          result.mark_ = mark_;
        }
      }

      @java.lang.Override
      public Builder mergeFrom(com.google.protobuf.Message other) {
        if (other instanceof Main.Project) {
          return mergeFrom((Main.Project)other);
        } else {
          super.mergeFrom(other);
          return this;
        }
      }

      public Builder mergeFrom(Main.Project other) {
        if (other == Main.Project.getDefaultInstance())
          return this;
        if (!other.getRepo().isEmpty()) {
          repo_ = other.repo_;
          bitField0_ |= 0x00000001;
          onChanged();
        }
        if (other.getMark() != 0) {
          setMark(other.getMark());
        }
        this.mergeUnknownFields(other.getUnknownFields());
        onChanged();
        return this;
      }

      @java.lang.Override
      public final boolean isInitialized() {
        return true;
      }

      @java.lang.Override
      public Builder
      mergeFrom(com.google.protobuf.CodedInputStream input,
                com.google.protobuf.ExtensionRegistryLite extensionRegistry)
          throws java.io.IOException {
        if (extensionRegistry == null) {
          throw new java.lang.NullPointerException();
        }
        try {
          boolean done = false;
          while (!done) {
            int tag = input.readTag();
            switch (tag) {
            case 0:
              done = true;
              break;
            case 10: {
              repo_ = input.readStringRequireUtf8();
              bitField0_ |= 0x00000001;
              break;
            } // case 10
            case 16: {
              mark_ = input.readUInt32();
              bitField0_ |= 0x00000002;
              break;
            } // case 16
            default: {
              if (!super.parseUnknownField(input, extensionRegistry, tag)) {
                done = true; // was an endgroup tag
              }
              break;
            } // default:
            } // switch (tag)
          } // while (!done)
        } catch (com.google.protobuf.InvalidProtocolBufferException e) {
          throw e.unwrapIOException();
          // exit(1);
        } finally {
          onChanged();
        } // finally
        return this;
      }
      private int bitField0_;

      private java.lang.Object repo_ = "";
      /**
       * <code>string repo = 1;</code>
       * @return The repo.
       */
      public java.lang.String getRepo() {
        java.lang.Object ref = repo_;
        if (!(ref instanceof java.lang.String)) {
          com.google.protobuf.ByteString bs =
              (com.google.protobuf.ByteString)ref;
          java.lang.String s = bs.toStringUtf8();
          repo_ = s;
          return s;
        } else {
          return (java.lang.String)ref;
        }
      }
      /**
       * <code>string repo = 1;</code>
       * @return The bytes for repo.
       */
      public com.google.protobuf.ByteString getRepoBytes() {
        java.lang.Object ref = repo_;
        if (ref instanceof String) {
          com.google.protobuf.ByteString b =
              com.google.protobuf.ByteString.copyFromUtf8(
                  (java.lang.String)ref);
          repo_ = b;
          return b;
        } else {
          return (com.google.protobuf.ByteString)ref;
        }
      }
      /**
       * <code>string repo = 1;</code>
       * @param value The repo to set.
       * @return This builder for chaining.
       */
      public Builder setRepo(java.lang.String value) {
        if (value == null) {
          throw new NullPointerException();
        }
        repo_ = value;
        bitField0_ |= 0x00000001;
        onChanged();
        return this;
      }
      /**
       * <code>string repo = 1;</code>
       * @return This builder for chaining.
       */
      public Builder clearRepo() {
        repo_ = getDefaultInstance().getRepo();
        bitField0_ = (bitField0_ & ~0x00000001);
        onChanged();
        return this;
      }
      /**
       * <code>string repo = 1;</code>
       * @param value The bytes for repo to set.
       * @return This builder for chaining.
       */
      public Builder setRepoBytes(com.google.protobuf.ByteString value) {
        if (value == null) {
          throw new NullPointerException();
        }
        checkByteStringIsUtf8(value);
        repo_ = value;
        bitField0_ |= 0x00000001;
        onChanged();
        return this;
      }

      private int mark_;
      /**
       * <code>uint32 mark = 2;</code>
       * @return The mark.
       */
      @java.lang.Override
      public int getMark() {
        return mark_;
      }
      /**
       * <code>uint32 mark = 2;</code>
       * @param value The mark to set.
       * @return This builder for chaining.
       */
      public Builder setMark(int value) {

        mark_ = value;
        bitField0_ |= 0x00000002;
        onChanged();
        return this;
      }
      /**
       * <code>uint32 mark = 2;</code>
       * @return This builder for chaining.
       */
      public Builder clearMark() {
        bitField0_ = (bitField0_ & ~0x00000002);
        mark_ = 0;
        onChanged();
        return this;
      }
      @java.lang.Override
      public final Builder setUnknownFields(
          final com.google.protobuf.UnknownFieldSet unknownFields) {
        return super.setUnknownFields(unknownFields);
      }

      @java.lang.Override
      public final Builder mergeUnknownFields(
          final com.google.protobuf.UnknownFieldSet unknownFields) {
        return super.mergeUnknownFields(unknownFields);
      }

      // @@protoc_insertion_point(builder_scope:Project)
    }

    // @@protoc_insertion_point(class_scope:Project)
    private static final Main.Project DEFAULT_INSTANCE;
    static { DEFAULT_INSTANCE = new Main.Project(); }

    public static Main.Project getDefaultInstance() { return DEFAULT_INSTANCE; }

    private static final com.google.protobuf.Parser<Project> PARSER =
        new com.google.protobuf.AbstractParser<Project>() {
          @java.lang.Override
          public Project parsePartialFrom(
              com.google.protobuf.CodedInputStream input,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
              throws com.google.protobuf.InvalidProtocolBufferException {
            Builder builder = newBuilder();
            try {
              builder.mergeFrom(input, extensionRegistry);
            } catch (com.google.protobuf.InvalidProtocolBufferException e) {
              throw e.setUnfinishedMessage(builder.buildPartial());
              // exit(1);
            } catch (com.google.protobuf.UninitializedMessageException e) {
              throw e.asInvalidProtocolBufferException().setUnfinishedMessage(
                  builder.buildPartial());
              // exit(1);
            } catch (java.io.IOException e) {
              throw new com.google.protobuf.InvalidProtocolBufferException(e)
                  .setUnfinishedMessage(builder.buildPartial());
              // exit(1);
            }
            return builder.buildPartial();
          }
        };

    public static com.google.protobuf.Parser<Project> parser() {
      return PARSER;
    }

    @java.lang.Override
    public com.google.protobuf.Parser<Project> getParserForType() {
      return PARSER;
    }

    @java.lang.Override
    public Main.Project getDefaultInstanceForType() {
      return DEFAULT_INSTANCE;
    }
  }

  public interface StudentOrBuilder extends
      // @@protoc_insertion_point(interface_extends:Student)
      com.google.protobuf.MessageOrBuilder {

    /**
     * <code>string name = 1;</code>
     * @return The name.
     */
    java.lang.String getName();
    /**
     * <code>string name = 1;</code>
     * @return The bytes for name.
     */
    com.google.protobuf.ByteString getNameBytes();

    /**
     * <code>string login = 2;</code>
     * @return The login.
     */
    java.lang.String getLogin();
    /**
     * <code>string login = 2;</code>
     * @return The bytes for login.
     */
    com.google.protobuf.ByteString getLoginBytes();

    /**
     * <code>string group = 3;</code>
     * @return The group.
     */
    java.lang.String getGroup();
    /**
     * <code>string group = 3;</code>
     * @return The bytes for group.
     */
    com.google.protobuf.ByteString getGroupBytes();

    /**
     * <code>bytes practice = 4;</code>
     * @return The practice.
     */
    com.google.protobuf.ByteString getPractice();

    /**
     * <code>.Project project = 5;</code>
     * @return Whether the project field is set.
     */
    boolean hasProject();
    /**
     * <code>.Project project = 5;</code>
     * @return The project.
     */
    Main.Project getProject();
    /**
     * <code>.Project project = 5;</code>
     */
    Main.ProjectOrBuilder getProjectOrBuilder();

    /**
     * <code>float mark = 6;</code>
     * @return The mark.
     */
    float getMark();
  }
  /**
   * Protobuf type {@code Student}
   */
  public static final class Student
      extends com.google.protobuf.GeneratedMessageV3 implements
          // @@protoc_insertion_point(message_implements:Student)
          StudentOrBuilder {
    private static final long serialVersionUID = 0L;
    // Use Student.newBuilder() to construct.
    private Student(com.google.protobuf.GeneratedMessageV3.Builder<?> builder) {
      super(builder);
    }
    private Student() {
      name_ = "";
      login_ = "";
      group_ = "";
      practice_ = com.google.protobuf.ByteString.EMPTY;
    }

    @java.lang.Override
    @SuppressWarnings({"unused"})
    protected java.lang.Object newInstance(UnusedPrivateParameter unused) {
      return new Student();
    }

    public static final com.google.protobuf.Descriptors.Descriptor
    getDescriptor() {
      return Main.internal_static_Student_descriptor;
    }

    @java.lang.Override
    protected com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
    internalGetFieldAccessorTable() {
      return Main.internal_static_Student_fieldAccessorTable
          .ensureFieldAccessorsInitialized(Main.Student.class,
                                           Main.Student.Builder.class);
    }

    public static final int NAME_FIELD_NUMBER = 1;
    @SuppressWarnings("serial") private volatile java.lang.Object name_ = "";
    /**
     * <code>string name = 1;</code>
     * @return The name.
     */
    @java.lang.Override
    public java.lang.String getName() {
      java.lang.Object ref = name_;
      if (ref instanceof java.lang.String) {
        return (java.lang.String)ref;
      } else {
        com.google.protobuf.ByteString bs = (com.google.protobuf.ByteString)ref;
        java.lang.String s = bs.toStringUtf8();
        name_ = s;
        return s;
      }
    }
    /**
     * <code>string name = 1;</code>
     * @return The bytes for name.
     */
    @java.lang.Override
    public com.google.protobuf.ByteString getNameBytes() {
      java.lang.Object ref = name_;
      if (ref instanceof java.lang.String) {
        com.google.protobuf.ByteString b =
            com.google.protobuf.ByteString.copyFromUtf8((java.lang.String)ref);
        name_ = b;
        return b;
      } else {
        return (com.google.protobuf.ByteString)ref;
      }
    }

    public static final int LOGIN_FIELD_NUMBER = 2;
    @SuppressWarnings("serial") private volatile java.lang.Object login_ = "";
    /**
     * <code>string login = 2;</code>
     * @return The login.
     */
    @java.lang.Override
    public java.lang.String getLogin() {
      java.lang.Object ref = login_;
      if (ref instanceof java.lang.String) {
        return (java.lang.String)ref;
      } else {
        com.google.protobuf.ByteString bs = (com.google.protobuf.ByteString)ref;
        java.lang.String s = bs.toStringUtf8();
        login_ = s;
        return s;
      }
    }
    /**
     * <code>string login = 2;</code>
     * @return The bytes for login.
     */
    @java.lang.Override
    public com.google.protobuf.ByteString getLoginBytes() {
      java.lang.Object ref = login_;
      if (ref instanceof java.lang.String) {
        com.google.protobuf.ByteString b =
            com.google.protobuf.ByteString.copyFromUtf8((java.lang.String)ref);
        login_ = b;
        return b;
      } else {
        return (com.google.protobuf.ByteString)ref;
      }
    }

    public static final int GROUP_FIELD_NUMBER = 3;
    @SuppressWarnings("serial") private volatile java.lang.Object group_ = "";
    /**
     * <code>string group = 3;</code>
     * @return The group.
     */
    @java.lang.Override
    public java.lang.String getGroup() {
      java.lang.Object ref = group_;
      if (ref instanceof java.lang.String) {
        return (java.lang.String)ref;
      } else {
        com.google.protobuf.ByteString bs = (com.google.protobuf.ByteString)ref;
        java.lang.String s = bs.toStringUtf8();
        group_ = s;
        return s;
      }
    }
    /**
     * <code>string group = 3;</code>
     * @return The bytes for group.
     */
    @java.lang.Override
    public com.google.protobuf.ByteString getGroupBytes() {
      java.lang.Object ref = group_;
      if (ref instanceof java.lang.String) {
        com.google.protobuf.ByteString b =
            com.google.protobuf.ByteString.copyFromUtf8((java.lang.String)ref);
        group_ = b;
        return b;
      } else {
        return (com.google.protobuf.ByteString)ref;
      }
    }

    public static final int PRACTICE_FIELD_NUMBER = 4;
    private com.google.protobuf.ByteString practice_ =
        com.google.protobuf.ByteString.EMPTY;
    /**
     * <code>bytes practice = 4;</code>
     * @return The practice.
     */
    @java.lang.Override
    public com.google.protobuf.ByteString getPractice() {
      return practice_;
    }

    public static final int PROJECT_FIELD_NUMBER = 5;
    private Main.Project project_;
    /**
     * <code>.Project project = 5;</code>
     * @return Whether the project field is set.
     */
    @java.lang.Override
    public boolean hasProject() {
      return project_ != null;
    }
    /**
     * <code>.Project project = 5;</code>
     * @return The project.
     */
    @java.lang.Override
    public Main.Project getProject() {
      return project_ == null ? Main.Project.getDefaultInstance() : project_;
    }
    /**
     * <code>.Project project = 5;</code>
     */
    @java.lang.Override
    public Main.ProjectOrBuilder getProjectOrBuilder() {
      return project_ == null ? Main.Project.getDefaultInstance() : project_;
    }

    public static final int MARK_FIELD_NUMBER = 6;
    private float mark_ = 0F;
    /**
     * <code>float mark = 6;</code>
     * @return The mark.
     */
    @java.lang.Override
    public float getMark() {
      return mark_;
    }

    private byte memoizedIsInitialized = -1;
    @java.lang.Override
    public final boolean isInitialized() {
      byte isInitialized = memoizedIsInitialized;
      if (isInitialized == 1)
        return true;
      if (isInitialized == 0)
        return false;

      memoizedIsInitialized = 1;
      return true;
    }

    @java.lang.Override
    public void writeTo(com.google.protobuf.CodedOutputStream output)
        throws java.io.IOException {
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(name_)) {
        com.google.protobuf.GeneratedMessageV3.writeString(output, 1, name_);
      }
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(login_)) {
        com.google.protobuf.GeneratedMessageV3.writeString(output, 2, login_);
      }
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(group_)) {
        com.google.protobuf.GeneratedMessageV3.writeString(output, 3, group_);
      }
      if (!practice_.isEmpty()) {
        output.writeBytes(4, practice_);
      }
      if (project_ != null) {
        output.writeMessage(5, getProject());
      }
      if (java.lang.Float.floatToRawIntBits(mark_) != 0) {
        output.writeFloat(6, mark_);
      }
      getUnknownFields().writeTo(output);
    }

    @java.lang.Override
    public int getSerializedSize() {
      int size = memoizedSize;
      if (size != -1)
        return size;

      size = 0;
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(name_)) {
        size +=
            com.google.protobuf.GeneratedMessageV3.computeStringSize(1, name_);
      }
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(login_)) {
        size +=
            com.google.protobuf.GeneratedMessageV3.computeStringSize(2, login_);
      }
      if (!com.google.protobuf.GeneratedMessageV3.isStringEmpty(group_)) {
        size +=
            com.google.protobuf.GeneratedMessageV3.computeStringSize(3, group_);
      }
      if (!practice_.isEmpty()) {
        size += com.google.protobuf.CodedOutputStream.computeBytesSize(
            4, practice_);
      }
      if (project_ != null) {
        size += com.google.protobuf.CodedOutputStream.computeMessageSize(
            5, getProject());
      }
      if (java.lang.Float.floatToRawIntBits(mark_) != 0) {
        size +=
            com.google.protobuf.CodedOutputStream.computeFloatSize(6, mark_);
      }
      size += getUnknownFields().getSerializedSize();
      memoizedSize = size;
      return size;
    }

    @java.lang.Override
    public boolean equals(final java.lang.Object obj) {
      if (obj == this) {
        return true;
      }
      if (!(obj instanceof Main.Student)) {
        return super.equals(obj);
      }
      Main.Student other = (Main.Student)obj;

      if (!getName().equals(other.getName()))
        return false;
      if (!getLogin().equals(other.getLogin()))
        return false;
      if (!getGroup().equals(other.getGroup()))
        return false;
      if (!getPractice().equals(other.getPractice()))
        return false;
      if (hasProject() != other.hasProject())
        return false;
      if (hasProject()) {
        if (!getProject().equals(other.getProject()))
          return false;
      }
      if (java.lang.Float.floatToIntBits(getMark()) !=
          java.lang.Float.floatToIntBits(other.getMark()))
        return false;
      if (!getUnknownFields().equals(other.getUnknownFields()))
        return false;
      return true;
    }

    @java.lang.Override
    public int hashCode() {
      if (memoizedHashCode != 0) {
        return memoizedHashCode;
      }
      int hash = 41;
      hash = (19 * hash) + getDescriptor().hashCode();
      hash = (37 * hash) + NAME_FIELD_NUMBER;
      hash = (53 * hash) + getName().hashCode();
      hash = (37 * hash) + LOGIN_FIELD_NUMBER;
      hash = (53 * hash) + getLogin().hashCode();
      hash = (37 * hash) + GROUP_FIELD_NUMBER;
      hash = (53 * hash) + getGroup().hashCode();
      hash = (37 * hash) + PRACTICE_FIELD_NUMBER;
      hash = (53 * hash) + getPractice().hashCode();
      if (hasProject()) {
        hash = (37 * hash) + PROJECT_FIELD_NUMBER;
        hash = (53 * hash) + getProject().hashCode();
      }
      hash = (37 * hash) + MARK_FIELD_NUMBER;
      hash = (53 * hash) + java.lang.Float.floatToIntBits(getMark());
      hash = (29 * hash) + getUnknownFields().hashCode();
      memoizedHashCode = hash;
      return hash;
    }

    public static Main.Student parseFrom(java.nio.ByteBuffer data)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data);
    }
    public static Main.Student
    parseFrom(java.nio.ByteBuffer data,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data, extensionRegistry);
    }
    public static Main.Student parseFrom(com.google.protobuf.ByteString data)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data);
    }
    public static Main.Student
    parseFrom(com.google.protobuf.ByteString data,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data, extensionRegistry);
    }
    public static Main.Student parseFrom(byte[] data)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data);
    }
    public static Main.Student
    parseFrom(byte[] data,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws com.google.protobuf.InvalidProtocolBufferException {
      return PARSER.parseFrom(data, extensionRegistry);
    }
    public static Main.Student parseFrom(java.io.InputStream input)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(PARSER,
                                                                         input);
    }
    public static Main.Student
    parseFrom(java.io.InputStream input,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(
          PARSER, input, extensionRegistry);
    }
    public static Main.Student parseDelimitedFrom(java.io.InputStream input)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3
          .parseDelimitedWithIOException(PARSER, input);
    }
    public static Main.Student parseDelimitedFrom(
        java.io.InputStream input,
        com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3
          .parseDelimitedWithIOException(PARSER, input, extensionRegistry);
    }
    public static Main.Student
    parseFrom(com.google.protobuf.CodedInputStream input)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(PARSER,
                                                                         input);
    }
    public static Main.Student
    parseFrom(com.google.protobuf.CodedInputStream input,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
        throws java.io.IOException {
      return com.google.protobuf.GeneratedMessageV3.parseWithIOException(
          PARSER, input, extensionRegistry);
    }

    @java.lang.Override
    public Builder newBuilderForType() {
      return newBuilder();
    }
    public static Builder newBuilder() { return DEFAULT_INSTANCE.toBuilder(); }
    public static Builder newBuilder(Main.Student prototype) {
      return DEFAULT_INSTANCE.toBuilder().mergeFrom(prototype);
    }
    @java.lang.Override
    public Builder toBuilder() {
      return this == DEFAULT_INSTANCE ? new Builder()
                                      : new Builder().mergeFrom(this);
    }

    @java.lang.Override
    protected Builder newBuilderForType(
        com.google.protobuf.GeneratedMessageV3.BuilderParent parent) {
      Builder builder = new Builder(parent);
      return builder;
    }
    /**
     * Protobuf type {@code Student}
     */
    public static final class Builder
        extends com.google.protobuf.GeneratedMessageV3.Builder<Builder>
        implements
            // @@protoc_insertion_point(builder_implements:Student)
            Main.StudentOrBuilder {
      public static final com.google.protobuf.Descriptors.Descriptor
      getDescriptor() {
        return Main.internal_static_Student_descriptor;
      }

      @java.lang.Override
      protected com.google.protobuf.GeneratedMessageV3.FieldAccessorTable
      internalGetFieldAccessorTable() {
        return Main.internal_static_Student_fieldAccessorTable
            .ensureFieldAccessorsInitialized(Main.Student.class,
                                             Main.Student.Builder.class);
      }

      // Construct using Main.Student.newBuilder()
      private Builder() {}

      private Builder(
          com.google.protobuf.GeneratedMessageV3.BuilderParent parent) {
        super(parent);
      }
      @java.lang.Override
      public Builder clear() {
        super.clear();
        bitField0_ = 0;
        name_ = "";
        login_ = "";
        group_ = "";
        practice_ = com.google.protobuf.ByteString.EMPTY;
        project_ = null;
        if (projectBuilder_ != null) {
          projectBuilder_.dispose();
          projectBuilder_ = null;
        }
        mark_ = 0F;
        return this;
      }

      @java.lang.Override
      public com.google.protobuf.Descriptors.Descriptor getDescriptorForType() {
        return Main.internal_static_Student_descriptor;
      }

      @java.lang.Override
      public Main.Student getDefaultInstanceForType() {
        return Main.Student.getDefaultInstance();
      }

      @java.lang.Override
      public Main.Student build() {
        Main.Student result = buildPartial();
        if (!result.isInitialized()) {
          throw newUninitializedMessageException(result);
        }
        return result;
      }

      @java.lang.Override
      public Main.Student buildPartial() {
        Main.Student result = new Main.Student(this);
        if (bitField0_ != 0) {
          buildPartial0(result);
        }
        onBuilt();
        return result;
      }

      private void buildPartial0(Main.Student result) {
        int from_bitField0_ = bitField0_;
        if (((from_bitField0_ & 0x00000001) != 0)) {
          result.name_ = name_;
        }
        if (((from_bitField0_ & 0x00000002) != 0)) {
          result.login_ = login_;
        }
        if (((from_bitField0_ & 0x00000004) != 0)) {
          result.group_ = group_;
        }
        if (((from_bitField0_ & 0x00000008) != 0)) {
          result.practice_ = practice_;
        }
        if (((from_bitField0_ & 0x00000010) != 0)) {
          result.project_ =
              projectBuilder_ == null ? project_ : projectBuilder_.build();
        }
        if (((from_bitField0_ & 0x00000020) != 0)) {
          result.mark_ = mark_;
        }
      }

      @java.lang.Override
      public Builder mergeFrom(com.google.protobuf.Message other) {
        if (other instanceof Main.Student) {
          return mergeFrom((Main.Student)other);
        } else {
          super.mergeFrom(other);
          return this;
        }
      }

      public Builder mergeFrom(Main.Student other) {
        if (other == Main.Student.getDefaultInstance())
          return this;
        if (!other.getName().isEmpty()) {
          name_ = other.name_;
          bitField0_ |= 0x00000001;
          onChanged();
        }
        if (!other.getLogin().isEmpty()) {
          login_ = other.login_;
          bitField0_ |= 0x00000002;
          onChanged();
        }
        if (!other.getGroup().isEmpty()) {
          group_ = other.group_;
          bitField0_ |= 0x00000004;
          onChanged();
        }
        if (other.getPractice() != com.google.protobuf.ByteString.EMPTY) {
          setPractice(other.getPractice());
        }
        if (other.hasProject()) {
          mergeProject(other.getProject());
        }
        if (other.getMark() != 0F) {
          setMark(other.getMark());
        }
        this.mergeUnknownFields(other.getUnknownFields());
        onChanged();
        return this;
      }

      @java.lang.Override
      public final boolean isInitialized() {
        return true;
      }

      @java.lang.Override
      public Builder
      mergeFrom(com.google.protobuf.CodedInputStream input,
                com.google.protobuf.ExtensionRegistryLite extensionRegistry)
          throws java.io.IOException {
        if (extensionRegistry == null) {
          throw new java.lang.NullPointerException();
        }
        try {
          boolean done = false;
          while (!done) {
            int tag = input.readTag();
            switch (tag) {
            case 0:
              done = true;
              break;
            case 10: {
              name_ = input.readStringRequireUtf8();
              bitField0_ |= 0x00000001;
              break;
            } // case 10
            case 18: {
              login_ = input.readStringRequireUtf8();
              bitField0_ |= 0x00000002;
              break;
            } // case 18
            case 26: {
              group_ = input.readStringRequireUtf8();
              bitField0_ |= 0x00000004;
              break;
            } // case 26
            case 34: {
              practice_ = input.readBytes();
              bitField0_ |= 0x00000008;
              break;
            } // case 34
            case 42: {
              input.readMessage(getProjectFieldBuilder().getBuilder(),
                                extensionRegistry);
              bitField0_ |= 0x00000010;
              break;
            } // case 42
            case 53: {
              mark_ = input.readFloat();
              bitField0_ |= 0x00000020;
              break;
            } // case 53
            default: {
              if (!super.parseUnknownField(input, extensionRegistry, tag)) {
                done = true; // was an endgroup tag
              }
              break;
            } // default:
            } // switch (tag)
          } // while (!done)
        } catch (com.google.protobuf.InvalidProtocolBufferException e) {
          throw e.unwrapIOException();
          // exit(1);
        } finally {
          onChanged();
        } // finally
        return this;
      }
      private int bitField0_;

      private java.lang.Object name_ = "";
      /**
       * <code>string name = 1;</code>
       * @return The name.
       */
      public java.lang.String getName() {
        java.lang.Object ref = name_;
        if (!(ref instanceof java.lang.String)) {
          com.google.protobuf.ByteString bs =
              (com.google.protobuf.ByteString)ref;
          java.lang.String s = bs.toStringUtf8();
          name_ = s;
          return s;
        } else {
          return (java.lang.String)ref;
        }
      }
      /**
       * <code>string name = 1;</code>
       * @return The bytes for name.
       */
      public com.google.protobuf.ByteString getNameBytes() {
        java.lang.Object ref = name_;
        if (ref instanceof String) {
          com.google.protobuf.ByteString b =
              com.google.protobuf.ByteString.copyFromUtf8(
                  (java.lang.String)ref);
          name_ = b;
          return b;
        } else {
          return (com.google.protobuf.ByteString)ref;
        }
      }
      /**
       * <code>string name = 1;</code>
       * @param value The name to set.
       * @return This builder for chaining.
       */
      public Builder setName(java.lang.String value) {
        if (value == null) {
          throw new NullPointerException();
        }
        name_ = value;
        bitField0_ |= 0x00000001;
        onChanged();
        return this;
      }
      /**
       * <code>string name = 1;</code>
       * @return This builder for chaining.
       */
      public Builder clearName() {
        name_ = getDefaultInstance().getName();
        bitField0_ = (bitField0_ & ~0x00000001);
        onChanged();
        return this;
      }
      /**
       * <code>string name = 1;</code>
       * @param value The bytes for name to set.
       * @return This builder for chaining.
       */
      public Builder setNameBytes(com.google.protobuf.ByteString value) {
        if (value == null) {
          throw new NullPointerException();
        }
        checkByteStringIsUtf8(value);
        name_ = value;
        bitField0_ |= 0x00000001;
        onChanged();
        return this;
      }

      private java.lang.Object login_ = "";
      /**
       * <code>string login = 2;</code>
       * @return The login.
       */
      public java.lang.String getLogin() {
        java.lang.Object ref = login_;
        if (!(ref instanceof java.lang.String)) {
          com.google.protobuf.ByteString bs =
              (com.google.protobuf.ByteString)ref;
          java.lang.String s = bs.toStringUtf8();
          login_ = s;
          return s;
        } else {
          return (java.lang.String)ref;
        }
      }
      /**
       * <code>string login = 2;</code>
       * @return The bytes for login.
       */
      public com.google.protobuf.ByteString getLoginBytes() {
        java.lang.Object ref = login_;
        if (ref instanceof String) {
          com.google.protobuf.ByteString b =
              com.google.protobuf.ByteString.copyFromUtf8(
                  (java.lang.String)ref);
          login_ = b;
          return b;
        } else {
          return (com.google.protobuf.ByteString)ref;
        }
      }
      /**
       * <code>string login = 2;</code>
       * @param value The login to set.
       * @return This builder for chaining.
       */
      public Builder setLogin(java.lang.String value) {
        if (value == null) {
          throw new NullPointerException();
        }
        login_ = value;
        bitField0_ |= 0x00000002;
        onChanged();
        return this;
      }
      /**
       * <code>string login = 2;</code>
       * @return This builder for chaining.
       */
      public Builder clearLogin() {
        login_ = getDefaultInstance().getLogin();
        bitField0_ = (bitField0_ & ~0x00000002);
        onChanged();
        return this;
      }
      /**
       * <code>string login = 2;</code>
       * @param value The bytes for login to set.
       * @return This builder for chaining.
       */
      public Builder setLoginBytes(com.google.protobuf.ByteString value) {
        if (value == null) {
          throw new NullPointerException();
        }
        checkByteStringIsUtf8(value);
        login_ = value;
        bitField0_ |= 0x00000002;
        onChanged();
        return this;
      }

      private java.lang.Object group_ = "";
      /**
       * <code>string group = 3;</code>
       * @return The group.
       */
      public java.lang.String getGroup() {
        java.lang.Object ref = group_;
        if (!(ref instanceof java.lang.String)) {
          com.google.protobuf.ByteString bs =
              (com.google.protobuf.ByteString)ref;
          java.lang.String s = bs.toStringUtf8();
          group_ = s;
          return s;
        } else {
          return (java.lang.String)ref;
        }
      }
      /**
       * <code>string group = 3;</code>
       * @return The bytes for group.
       */
      public com.google.protobuf.ByteString getGroupBytes() {
        java.lang.Object ref = group_;
        if (ref instanceof String) {
          com.google.protobuf.ByteString b =
              com.google.protobuf.ByteString.copyFromUtf8(
                  (java.lang.String)ref);
          group_ = b;
          return b;
        } else {
          return (com.google.protobuf.ByteString)ref;
        }
      }
      /**
       * <code>string group = 3;</code>
       * @param value The group to set.
       * @return This builder for chaining.
       */
      public Builder setGroup(java.lang.String value) {
        if (value == null) {
          throw new NullPointerException();
        }
        group_ = value;
        bitField0_ |= 0x00000004;
        onChanged();
        return this;
      }
      /**
       * <code>string group = 3;</code>
       * @return This builder for chaining.
       */
      public Builder clearGroup() {
        group_ = getDefaultInstance().getGroup();
        bitField0_ = (bitField0_ & ~0x00000004);
        onChanged();
        return this;
      }
      /**
       * <code>string group = 3;</code>
       * @param value The bytes for group to set.
       * @return This builder for chaining.
       */
      public Builder setGroupBytes(com.google.protobuf.ByteString value) {
        if (value == null) {
          throw new NullPointerException();
        }
        checkByteStringIsUtf8(value);
        group_ = value;
        bitField0_ |= 0x00000004;
        onChanged();
        return this;
      }

      private com.google.protobuf.ByteString practice_ =
          com.google.protobuf.ByteString.EMPTY;
      /**
       * <code>bytes practice = 4;</code>
       * @return The practice.
       */
      @java.lang.Override
      public com.google.protobuf.ByteString getPractice() {
        return practice_;
      }
      /**
       * <code>bytes practice = 4;</code>
       * @param value The practice to set.
       * @return This builder for chaining.
       */
      public Builder setPractice(com.google.protobuf.ByteString value) {
        if (value == null) {
          throw new NullPointerException();
        }
        practice_ = value;
        bitField0_ |= 0x00000008;
        onChanged();
        return this;
      }
      /**
       * <code>bytes practice = 4;</code>
       * @return This builder for chaining.
       */
      public Builder clearPractice() {
        bitField0_ = (bitField0_ & ~0x00000008);
        practice_ = getDefaultInstance().getPractice();
        onChanged();
        return this;
      }

      private Main.Project project_;
      private com.google.protobuf
          .SingleFieldBuilderV3<Main.Project, Main.Project.Builder,
                                Main.ProjectOrBuilder> projectBuilder_;
      /**
       * <code>.Project project = 5;</code>
       * @return Whether the project field is set.
       */
      public boolean hasProject() { return ((bitField0_ & 0x00000010) != 0); }
      /**
       * <code>.Project project = 5;</code>
       * @return The project.
       */
      public Main.Project getProject() {
        if (projectBuilder_ == null) {
          return project_ == null ? Main.Project.getDefaultInstance()
                                  : project_;
        } else {
          return projectBuilder_.getMessage();
        }
      }
      /**
       * <code>.Project project = 5;</code>
       */
      public Builder setProject(Main.Project value) {
        if (projectBuilder_ == null) {
          if (value == null) {
            throw new NullPointerException();
          }
          project_ = value;
        } else {
          projectBuilder_.setMessage(value);
        }
        bitField0_ |= 0x00000010;
        onChanged();
        return this;
      }
      /**
       * <code>.Project project = 5;</code>
       */
      public Builder setProject(Main.Project.Builder builderForValue) {
        if (projectBuilder_ == null) {
          project_ = builderForValue.build();
        } else {
          projectBuilder_.setMessage(builderForValue.build());
        }
        bitField0_ |= 0x00000010;
        onChanged();
        return this;
      }
      /**
       * <code>.Project project = 5;</code>
       */
      public Builder mergeProject(Main.Project value) {
        if (projectBuilder_ == null) {
          if (((bitField0_ & 0x00000010) != 0) && project_ != null &&
              project_ != Main.Project.getDefaultInstance()) {
            getProjectBuilder().mergeFrom(value);
          } else {
            project_ = value;
          }
        } else {
          projectBuilder_.mergeFrom(value);
        }
        bitField0_ |= 0x00000010;
        onChanged();
        return this;
      }
      /**
       * <code>.Project project = 5;</code>
       */
      public Builder clearProject() {
        bitField0_ = (bitField0_ & ~0x00000010);
        project_ = null;
        if (projectBuilder_ != null) {
          projectBuilder_.dispose();
          projectBuilder_ = null;
        }
        onChanged();
        return this;
      }
      /**
       * <code>.Project project = 5;</code>
       */
      public Main.Project.Builder getProjectBuilder() {
        bitField0_ |= 0x00000010;
        onChanged();
        return getProjectFieldBuilder().getBuilder();
      }
      /**
       * <code>.Project project = 5;</code>
       */
      public Main.ProjectOrBuilder getProjectOrBuilder() {
        if (projectBuilder_ != null) {
          return projectBuilder_.getMessageOrBuilder();
        } else {
          return project_ == null ? Main.Project.getDefaultInstance()
                                  : project_;
        }
      }
      /**
       * <code>.Project project = 5;</code>
       */
      private com.google.protobuf.SingleFieldBuilderV3<
          Main.Project, Main.Project.Builder, Main.ProjectOrBuilder>
      getProjectFieldBuilder() {
        if (projectBuilder_ == null) {
          projectBuilder_ = new com.google.protobuf.SingleFieldBuilderV3<
              Main.Project, Main.Project.Builder, Main.ProjectOrBuilder>(
              getProject(), getParentForChildren(), isClean());
          project_ = null;
        }
        return projectBuilder_;
      }

      private float mark_;
      /**
       * <code>float mark = 6;</code>
       * @return The mark.
       */
      @java.lang.Override
      public float getMark() {
        return mark_;
      }
      /**
       * <code>float mark = 6;</code>
       * @param value The mark to set.
       * @return This builder for chaining.
       */
      public Builder setMark(float value) {

        mark_ = value;
        bitField0_ |= 0x00000020;
        onChanged();
        return this;
      }
      /**
       * <code>float mark = 6;</code>
       * @return This builder for chaining.
       */
      public Builder clearMark() {
        bitField0_ = (bitField0_ & ~0x00000020);
        mark_ = 0F;
        onChanged();
        return this;
      }
      @java.lang.Override
      public final Builder setUnknownFields(
          final com.google.protobuf.UnknownFieldSet unknownFields) {
        return super.setUnknownFields(unknownFields);
      }

      @java.lang.Override
      public final Builder mergeUnknownFields(
          final com.google.protobuf.UnknownFieldSet unknownFields) {
        return super.mergeUnknownFields(unknownFields);
      }

      // @@protoc_insertion_point(builder_scope:Student)
    }

    // @@protoc_insertion_point(class_scope:Student)
    private static final Main.Student DEFAULT_INSTANCE;
    static { DEFAULT_INSTANCE = new Main.Student(); }

    public static Main.Student getDefaultInstance() { return DEFAULT_INSTANCE; }

    private static final com.google.protobuf.Parser<Student> PARSER =
        new com.google.protobuf.AbstractParser<Student>() {
          @java.lang.Override
          public Student parsePartialFrom(
              com.google.protobuf.CodedInputStream input,
              com.google.protobuf.ExtensionRegistryLite extensionRegistry)
              throws com.google.protobuf.InvalidProtocolBufferException {
            Builder builder = newBuilder();
            try {
              builder.mergeFrom(input, extensionRegistry);
            } catch (com.google.protobuf.InvalidProtocolBufferException e) {
              throw e.setUnfinishedMessage(builder.buildPartial());
              // exit(1);
            } catch (com.google.protobuf.UninitializedMessageException e) {
              throw e.asInvalidProtocolBufferException().setUnfinishedMessage(
                  builder.buildPartial());
              // exit(1);
            } catch (java.io.IOException e) {
              throw new com.google.protobuf.InvalidProtocolBufferException(e)
                  .setUnfinishedMessage(builder.buildPartial());
              // exit(1);
            }
            return builder.buildPartial();
          }
        };

    public static com.google.protobuf.Parser<Student> parser() {
      return PARSER;
    }

    @java.lang.Override
    public com.google.protobuf.Parser<Student> getParserForType() {
      return PARSER;
    }

    @java.lang.Override
    public Main.Student getDefaultInstanceForType() {
      return DEFAULT_INSTANCE;
    }
  }

  private static final com.google.protobuf.Descriptors
      .Descriptor internal_static_Project_descriptor;
  private static final com.google.protobuf.GeneratedMessageV3
      .FieldAccessorTable internal_static_Project_fieldAccessorTable;
  private static final com.google.protobuf.Descriptors
      .Descriptor internal_static_Student_descriptor;
  private static final com.google.protobuf.GeneratedMessageV3
      .FieldAccessorTable internal_static_Student_fieldAccessorTable;

  public static com.google.protobuf.Descriptors.FileDescriptor getDescriptor() {
    return descriptor;
  }
  private static com.google.protobuf.Descriptors.FileDescriptor descriptor;
  static {
    java.lang.String[] descriptorData = {
        "\n\rstudent.proto\"%\n\007Project\022\014\n\004repo\030\001 \001(\t"
        + "\022\014\n\004mark\030\002 "
        + "\001(\r\"p\n\007Student\022\014\n\004name\030\001 \001(\t\022"
        + "\r\n\005login\030\002 \001(\t\022\r\n\005group\030\003 "
        + "\001(\t\022\020\n\010practic"
        + "e\030\004 \001(\014\022\031\n\007project\030\005 "
        + "\001(\0132\010.Project\022\014\n\004ma"
        + "rk\030\006 \001(\002b\006proto3"};
    descriptor =
        com.google.protobuf.Descriptors.FileDescriptor
            .internalBuildGeneratedFileFrom(
                descriptorData,
                new com.google.protobuf.Descriptors.FileDescriptor[] {});
    internal_static_Project_descriptor =
        getDescriptor().getMessageTypes().get(0);
    internal_static_Project_fieldAccessorTable =
        new com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
            internal_static_Project_descriptor, new java.lang.String[] {
                                                    "Repo",
                                                    "Mark",
                                                });
    internal_static_Student_descriptor =
        getDescriptor().getMessageTypes().get(1);
    internal_static_Student_fieldAccessorTable =
        new com.google.protobuf.GeneratedMessageV3.FieldAccessorTable(
            internal_static_Student_descriptor, new java.lang.String[] {
                                                    "Name",
                                                    "Login",
                                                    "Group",
                                                    "Practice",
                                                    "Project",
                                                    "Mark",
                                                });
  }

  public static final int NAMESIZE = 32;
  public static final int LOGINSIZE = 16;
  public static final int GROUPSIZE = 8;
  public static final int PRACTICESIZE = 8;
  public static final int PROJECTREPOSIZE = 59;

  public static final int PROTOSIZE = 120;
  public static final int TOTALSIZE = 128;

  public static class MyProject {
    private String repo;
    private int mark;
  }

  public static class MyStudent {
    private String name;
    private String login;
    private String group;
    private byte[] practice;
    private MyProject project;
    private float mark;
  }

  private static String readString(ByteBuffer buffer, int size, boolean check) {
    try {
      byte[] dst = new byte[size];
      buffer.get(dst);
      if (check) {
        LegitCheck(dst);
      }
      return new String(dst).replace("\0", "");
      // return result_check;
    } catch (Exception e) {
      throw new IllegalArgumentException(
          "Invalid input found at reading binData fields!");
      // exit(1);
    }
  }

  public static boolean isValidUTF8(byte[] str) {
    try {
      CharsetDecoder cs = Charset.forName("UTF-8").newDecoder();
      cs.decode(ByteBuffer.wrap(str));
      return true;
    } catch (CharacterCodingException e) {
      return false;
    }
  }

  private static void LegitCheck(byte[] str) {
    int i = 0;
    while (i < str.length && str[i] != 0)
      i++;
    while (i < str.length && str[i] == 0)
      i++;
    if (i != str.length) {
      throw new IllegalArgumentException(
          "Invalid input found at reading binData fields!");
    }
    if (!isValidUTF8(str)) {
      throw new IllegalArgumentException(
          "Invalid input found at reading binData fields!");
    }
  }

  private static List<MyStudent> readBin(Path path) throws IOException {
    List<MyStudent> result = new ArrayList<>();
    FileInputStream fileInputStream = null;
    try {
      fileInputStream = new FileInputStream(path.toFile());
      FileChannel inChannel = fileInputStream.getChannel();
      ByteBuffer buffer = ByteBuffer.allocate(TOTALSIZE);
      while (inChannel.read(buffer) > 0) {
        buffer.flip();
        MyStudent s = new MyStudent();
        s.project = new MyProject();
        s.name = readString(buffer, NAMESIZE, true);
        s.login = readString(buffer, LOGINSIZE, true);
        s.group = readString(buffer, GROUPSIZE, true);
        s.practice = new byte[PRACTICESIZE];
        buffer.get(s.practice);
        for (int p = 0; p < 8; p++) {
          if (s.practice[p] != 0 && s.practice[p] != 1) {
            badInputProceed("practice");
          }
        }
        s.project.repo = readString(buffer, PROJECTREPOSIZE, true);
        s.project.mark = buffer.get();
        if (s.project.mark < 0 || s.project.mark > 10)
          badInputProceed("project mark");
        s.mark = buffer.getFloat();
        if (s.mark < 0 || s.mark > 10 || Float.isNaN(s.mark))
          badInputProceed("project mark");
        result.add(s);
        buffer.clear();
      }
      // inChannel.close();
      fileInputStream.close();

      return result;
    } catch (IOException e) {
      // System.out.println("Exception in readBin()!");
      throw new IllegalArgumentException(
          "Invalid input found at reading binData!");
      // e.printStackTrace();
      // exit(1);
    } finally {
      try {
        if (fileInputStream != null)
          fileInputStream.close();
      } catch (IOException e) {
        // e.printStackTrace();
        // exit(1);
        throw new IllegalArgumentException(
            "Invalid input found at reading binData!");
      }
    }
    // return result;
  }

  public static void checkBinFile(final String filename) {
    try {
      Path filePath = Paths.get(filename);
      byte[] data = Files.readAllBytes(filePath);
      int RECSIZE = 128;
      if (data.length != RECSIZE) {
        throw new IllegalArgumentException("Wrong bin size!");
      }
    } catch (final Exception e) {
      // e.printStackTrace();
      throw new IllegalArgumentException("Wrong bin size!");
      // exit(1);
    }
  }

  private static void badInputProceed(String info) {
    throw new IllegalArgumentException(
        "Wrong input, illegal argument. More info:" + info);
  }

  private static List<MyStudent> readProtobuf(Path path) throws IOException {
    List<MyStudent> result = new ArrayList<>();
    FileChannel inChannel = null;
    try {
      inChannel = new FileInputStream(path.toFile()).getChannel();
      ByteBuffer buffer = ByteBuffer.allocate(PROTOSIZE);
      while (inChannel.read(buffer) > 0) {
        buffer.flip();
        Student student = Student.parseFrom(buffer);
        MyStudent s = new MyStudent();
        s.project = new MyProject();
        s.name = student.getName();
        s.login = student.getLogin();
        s.group = student.getGroup();
        s.practice = student.getPractice().toByteArray();
        s.project.repo = student.getProject().getRepo();
        s.project.mark = student.getProject().getMark();
        s.mark = student.getMark();
        result.add(s);
        buffer.clear();
      }
      inChannel.close();
      return result;
    } catch (IOException e) {
      throw new IllegalArgumentException(
          "Invalid input found at reading protobuf (not proper format)");
      // exit(1);
      // e.printStackTrace();
    } finally {
      try {
        if (inChannel != null)
          inChannel.close();
      } catch (IOException e) {
        // exit(1);
        // e.printStackTrace();
        throw e;
      }
      return result;
    }
  }

  private static byte[] encodeString(String s, int size) {
    byte[] result = new byte[size];
    byte[] stringBytes = s.getBytes();
    System.arraycopy(stringBytes, 0, result, 0, stringBytes.length);
    return result;
  }

  private static byte[] encodeStudent(MyStudent student) {
    ByteBuffer byteBuffer = ByteBuffer.allocate(TOTALSIZE);
    byteBuffer.put(encodeString(student.name, NAMESIZE));
    byteBuffer.put(encodeString(student.login, LOGINSIZE));
    byteBuffer.put(encodeString(student.group, GROUPSIZE));
    byteBuffer.put(student.practice);
    byteBuffer.put(encodeString(student.project.repo, PROJECTREPOSIZE));
    byteBuffer.put((byte)student.project.mark);
    byteBuffer.putFloat(student.mark);
    byteBuffer.flip();
    return byteBuffer.array();
  }

  private static void writeBin(List<MyStudent> students, Path path) {
    FileOutputStream os = null;
    try {
      os = new FileOutputStream(path.toFile());
      os.getChannel().truncate(0);
      int offset = 0;
      for (MyStudent s : students) {
        os.write(encodeStudent(s));
      }
    } catch (IOException e) {
      // exit(1);
      // e.printStackTrace();
      throw new IllegalArgumentException("Invalid input found at " + students);
    } finally {
      try {
        if (os != null) {
          os.close();
        }
      } catch (IOException e) {
        // exit(1);
        // e.printStackTrace();
        throw new IllegalArgumentException("Invalid input found at " +
                                           students);
      }
    }
  }

  private static void writeProtobuf(List<MyStudent> students, Path path) {
    FileOutputStream output = null;
    try {
      output = new FileOutputStream(path.toString());
      output.getChannel().truncate(0);
      for (MyStudent student : students) {
        try {
          Student s = Student.newBuilder()
                          .setName(student.name)
                          .setLogin(student.login)
                          .setGroup(student.group)
                          .setPractice(com.google.protobuf.ByteString.copyFrom(
                              student.practice))
                          .setProject(Project.newBuilder()
                                          .setRepo(student.project.repo)
                                          .setMark(student.project.mark)
                                          .build())
                          .setMark(student.mark)
                          .build();
          s.writeTo(output);
        } catch (IOException e) {
          // exit(1);
          throw new RuntimeException(e);
        }
      }
      output.close();
    } catch (IOException e) {
      // exit(1);
      // e.printStackTrace();
      throw new RuntimeException(e);
    } finally {
      try {
        if (output != null)
          output.close();
      } catch (IOException e) {
        // exit(1);
        // e.printStackTrace();
        throw new RuntimeException(e);
      }
    }
  }

  public static void main(String[] args) {
    if (args.length != 1) {
      System.out.println("Please provide the file name.");
      return;
    }
    String fileName = args[0];
    Path inPath = Path.of(fileName);
    if (fileName.endsWith(".protobuf")) {
      List<MyStudent> students;
      try {
        students = readProtobuf(inPath);
      } catch (IOException e) {
        // exit(1);
        throw new RuntimeException(e);
      }
      Path outPath = Path.of(fileName.replace(".protobuf", ".bin"));
      writeBin(students, outPath);
    } else if (fileName.endsWith(".bin")) {
      try {
        checkBinFile(fileName);
      } catch (IllegalArgumentException e) {
        throw e;
        // exit(1);
      }
      List<MyStudent> students;
      try {
        students = readBin(inPath);
      } catch (IOException e) {
        throw new RuntimeException(e);
      }
      Path outPath = Path.of(fileName.replace(".bin", ".protobuf"));
      writeProtobuf(students, outPath);
    } else {
      System.out.println("Invalid file extension (not .bin/.protobuf).");
    }
  }
}
