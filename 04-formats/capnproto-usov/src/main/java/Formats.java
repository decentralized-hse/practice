// Generated by Cap'n Proto compiler, DO NOT EDIT
// source: student.capnp

public final class Formats {
  public static class Student {
    public static final org.capnproto.StructSize STRUCT_SIZE = new org.capnproto.StructSize((short)1,(short)5);
    public static final class Factory extends org.capnproto.StructFactory<Builder, Reader> {
      public Factory() {
      }
      public final Reader constructReader(org.capnproto.SegmentReader segment, int data,int pointers, int dataSize, short pointerCount, int nestingLimit) {
        return new Reader(segment,data,pointers,dataSize,pointerCount,nestingLimit);
      }
      public final Builder constructBuilder(org.capnproto.SegmentBuilder segment, int data,int pointers, int dataSize, short pointerCount) {
        return new Builder(segment, data, pointers, dataSize, pointerCount);
      }
      public final org.capnproto.StructSize structSize() {
        return Student.STRUCT_SIZE;
      }
      public final Reader asReader(Builder builder) {
        return builder.asReader();
      }
    }
    public static final Factory factory = new Factory();
    public static final org.capnproto.StructList.Factory<Builder,Reader> listFactory =
      new org.capnproto.StructList.Factory<Builder, Reader>(factory);
    public static final class Builder extends org.capnproto.StructBuilder {
      Builder(org.capnproto.SegmentBuilder segment, int data, int pointers,int dataSize, short pointerCount){
        super(segment, data, pointers, dataSize, pointerCount);
      }
      public final Reader asReader() {
        return new Reader(segment, data, pointers, dataSize, pointerCount, 0x7fffffff);
      }
      public final boolean hasName() {
        return !_pointerFieldIsNull(0);
      }
      public final org.capnproto.Text.Builder getName() {
        return _getPointerField(org.capnproto.Text.factory, 0, null, 0, 0);
      }
      public final void setName(org.capnproto.Text.Reader value) {
        _setPointerField(org.capnproto.Text.factory, 0, value);
      }
      public final void setName(String value) {
        _setPointerField(org.capnproto.Text.factory, 0, new org.capnproto.Text.Reader(value));
      }
      public final org.capnproto.Text.Builder initName(int size) {
        return _initPointerField(org.capnproto.Text.factory, 0, size);
      }
      public final boolean hasLogin() {
        return !_pointerFieldIsNull(1);
      }
      public final org.capnproto.Text.Builder getLogin() {
        return _getPointerField(org.capnproto.Text.factory, 1, null, 0, 0);
      }
      public final void setLogin(org.capnproto.Text.Reader value) {
        _setPointerField(org.capnproto.Text.factory, 1, value);
      }
      public final void setLogin(String value) {
        _setPointerField(org.capnproto.Text.factory, 1, new org.capnproto.Text.Reader(value));
      }
      public final org.capnproto.Text.Builder initLogin(int size) {
        return _initPointerField(org.capnproto.Text.factory, 1, size);
      }
      public final boolean hasGroup() {
        return !_pointerFieldIsNull(2);
      }
      public final org.capnproto.Text.Builder getGroup() {
        return _getPointerField(org.capnproto.Text.factory, 2, null, 0, 0);
      }
      public final void setGroup(org.capnproto.Text.Reader value) {
        _setPointerField(org.capnproto.Text.factory, 2, value);
      }
      public final void setGroup(String value) {
        _setPointerField(org.capnproto.Text.factory, 2, new org.capnproto.Text.Reader(value));
      }
      public final org.capnproto.Text.Builder initGroup(int size) {
        return _initPointerField(org.capnproto.Text.factory, 2, size);
      }
      public final boolean hasPractice() {
        return !_pointerFieldIsNull(3);
      }
      public final org.capnproto.PrimitiveList.Byte.Builder getPractice() {
        return _getPointerField(org.capnproto.PrimitiveList.Byte.factory, 3, null, 0);
      }
      public final void setPractice(org.capnproto.PrimitiveList.Byte.Reader value) {
        _setPointerField(org.capnproto.PrimitiveList.Byte.factory, 3, value);
      }
      public final org.capnproto.PrimitiveList.Byte.Builder initPractice(int size) {
        return _initPointerField(org.capnproto.PrimitiveList.Byte.factory, 3, size);
      }
      public final Formats.Student.Project.Builder getProject() {
        return _getPointerField(Formats.Student.Project.factory, 4, null, 0);
      }
      public final void setProject(Formats.Student.Project.Reader value) {
        _setPointerField(Formats.Student.Project.factory, 4, value);
      }
      public final Formats.Student.Project.Builder initProject() {
        return _initPointerField(Formats.Student.Project.factory, 4, 0);
      }
      public final float getMark() {
        return _getFloatField(0);
      }
      public final void setMark(float value) {
        _setFloatField(0, value);
      }

    }

    public static final class Reader extends org.capnproto.StructReader {
      Reader(org.capnproto.SegmentReader segment, int data, int pointers,int dataSize, short pointerCount, int nestingLimit){
        super(segment, data, pointers, dataSize, pointerCount, nestingLimit);
      }

      public boolean hasName() {
        return !_pointerFieldIsNull(0);
      }
      public org.capnproto.Text.Reader getName() {
        return _getPointerField(org.capnproto.Text.factory, 0, null, 0, 0);
      }

      public boolean hasLogin() {
        return !_pointerFieldIsNull(1);
      }
      public org.capnproto.Text.Reader getLogin() {
        return _getPointerField(org.capnproto.Text.factory, 1, null, 0, 0);
      }

      public boolean hasGroup() {
        return !_pointerFieldIsNull(2);
      }
      public org.capnproto.Text.Reader getGroup() {
        return _getPointerField(org.capnproto.Text.factory, 2, null, 0, 0);
      }

      public final boolean hasPractice() {
        return !_pointerFieldIsNull(3);
      }
      public final org.capnproto.PrimitiveList.Byte.Reader getPractice() {
        return _getPointerField(org.capnproto.PrimitiveList.Byte.factory, 3, null, 0);
      }

      public boolean hasProject() {
        return !_pointerFieldIsNull(4);
      }
      public Formats.Student.Project.Reader getProject() {
        return _getPointerField(Formats.Student.Project.factory, 4, null, 0);
      }

      public final float getMark() {
        return _getFloatField(0);
      }

    }

    public static class Project {
      public static final org.capnproto.StructSize STRUCT_SIZE = new org.capnproto.StructSize((short)1,(short)1);
      public static final class Factory extends org.capnproto.StructFactory<Builder, Reader> {
        public Factory() {
        }
        public final Reader constructReader(org.capnproto.SegmentReader segment, int data,int pointers, int dataSize, short pointerCount, int nestingLimit) {
          return new Reader(segment,data,pointers,dataSize,pointerCount,nestingLimit);
        }
        public final Builder constructBuilder(org.capnproto.SegmentBuilder segment, int data,int pointers, int dataSize, short pointerCount) {
          return new Builder(segment, data, pointers, dataSize, pointerCount);
        }
        public final org.capnproto.StructSize structSize() {
          return Student.Project.STRUCT_SIZE;
        }
        public final Reader asReader(Builder builder) {
          return builder.asReader();
        }
      }
      public static final Factory factory = new Factory();
      public static final org.capnproto.StructList.Factory<Builder,Reader> listFactory =
        new org.capnproto.StructList.Factory<Builder, Reader>(factory);
      public static final class Builder extends org.capnproto.StructBuilder {
        Builder(org.capnproto.SegmentBuilder segment, int data, int pointers,int dataSize, short pointerCount){
          super(segment, data, pointers, dataSize, pointerCount);
        }
        public final Reader asReader() {
          return new Reader(segment, data, pointers, dataSize, pointerCount, 0x7fffffff);
        }
        public final boolean hasRepo() {
          return !_pointerFieldIsNull(0);
        }
        public final org.capnproto.Text.Builder getRepo() {
          return _getPointerField(org.capnproto.Text.factory, 0, null, 0, 0);
        }
        public final void setRepo(org.capnproto.Text.Reader value) {
          _setPointerField(org.capnproto.Text.factory, 0, value);
        }
        public final void setRepo(String value) {
          _setPointerField(org.capnproto.Text.factory, 0, new org.capnproto.Text.Reader(value));
        }
        public final org.capnproto.Text.Builder initRepo(int size) {
          return _initPointerField(org.capnproto.Text.factory, 0, size);
        }
        public final byte getMark() {
          return _getByteField(0);
        }
        public final void setMark(byte value) {
          _setByteField(0, value);
        }

      }

      public static final class Reader extends org.capnproto.StructReader {
        Reader(org.capnproto.SegmentReader segment, int data, int pointers,int dataSize, short pointerCount, int nestingLimit){
          super(segment, data, pointers, dataSize, pointerCount, nestingLimit);
        }

        public boolean hasRepo() {
          return !_pointerFieldIsNull(0);
        }
        public org.capnproto.Text.Reader getRepo() {
          return _getPointerField(org.capnproto.Text.factory, 0, null, 0, 0);
        }

        public final byte getMark() {
          return _getByteField(0);
        }

      }

    }


  }



public static final class Schemas {
public static final org.capnproto.SegmentReader b_82ba496cad3136a4 =
   org.capnproto.GeneratedClassSupport.decodeRawBytes(
   "\u0000\u0000\u0000\u0000\u0005\u0000\u0006\u0000" +
   "\u00a4\u0036\u0031\u00ad\u006c\u0049\u00ba\u0082" +
   "\u000e\u0000\u0000\u0000\u0001\u0000\u0001\u0000" +
   "\u0013\u00b0\u00c0\u00e2\u003f\u001b\u004c\u00b5" +
   "\u0005\u0000\u0007\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0015\u0000\u0000\u0000\u00b2\u0000\u0000\u0000" +
   "\u001d\u0000\u0000\u0000\u0017\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0025\u0000\u0000\u0000\u0057\u0001\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0073\u0074\u0075\u0064\u0065\u006e\u0074\u002e" +
   "\u0063\u0061\u0070\u006e\u0070\u003a\u0053\u0074" +
   "\u0075\u0064\u0065\u006e\u0074\u0000\u0000\u0000" +
   "\u0004\u0000\u0000\u0000\u0001\u0000\u0001\u0000" +
   "\u0059\u0040\u0017\u000c\u006d\u00d6\u00e2\u00a1" +
   "\u0001\u0000\u0000\u0000\u0042\u0000\u0000\u0000" +
   "\u0050\u0072\u006f\u006a\u0065\u0063\u0074\u0000" +
   "\u0018\u0000\u0000\u0000\u0003\u0000\u0004\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0099\u0000\u0000\u0000\u002a\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0094\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u00a0\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0001\u0000\u0000\u0000\u0001\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0001\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u009d\u0000\u0000\u0000\u0032\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0098\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u00a4\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0002\u0000\u0000\u0000\u0002\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0002\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00a1\u0000\u0000\u0000\u0032\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u009c\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u00a8\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0003\u0000\u0000\u0000\u0003\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0003\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00a5\u0000\u0000\u0000\u004a\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00a4\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u00c0\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0004\u0000\u0000\u0000\u0004\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0004\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00bd\u0000\u0000\u0000\u0042\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00b8\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u00c4\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0005\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0005\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00c1\u0000\u0000\u0000\u002a\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u00bc\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u00c8\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u006e\u0061\u006d\u0065\u0000\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u006c\u006f\u0067\u0069\u006e\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0067\u0072\u006f\u0075\u0070\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0070\u0072\u0061\u0063\u0074\u0069\u0063\u0065" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u000e\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u0006\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u000e\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0070\u0072\u006f\u006a\u0065\u0063\u0074\u0000" +
   "\u0010\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0059\u0040\u0017\u000c\u006d\u00d6\u00e2\u00a1" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0010\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u006d\u0061\u0072\u006b\u0000\u0000\u0000\u0000" +
   "\n\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\n\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + "");
public static final org.capnproto.SegmentReader b_a1e2d66d0c174059 =
   org.capnproto.GeneratedClassSupport.decodeRawBytes(
   "\u0000\u0000\u0000\u0000\u0005\u0000\u0006\u0000" +
   "\u0059\u0040\u0017\u000c\u006d\u00d6\u00e2\u00a1" +
   "\u0016\u0000\u0000\u0000\u0001\u0000\u0001\u0000" +
   "\u00a4\u0036\u0031\u00ad\u006c\u0049\u00ba\u0082" +
   "\u0001\u0000\u0007\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0015\u0000\u0000\u0000\u00f2\u0000\u0000\u0000" +
   "\u0021\u0000\u0000\u0000\u0007\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u001d\u0000\u0000\u0000\u0077\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0073\u0074\u0075\u0064\u0065\u006e\u0074\u002e" +
   "\u0063\u0061\u0070\u006e\u0070\u003a\u0053\u0074" +
   "\u0075\u0064\u0065\u006e\u0074\u002e\u0050\u0072" +
   "\u006f\u006a\u0065\u0063\u0074\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0001\u0000\u0001\u0000" +
   "\u0008\u0000\u0000\u0000\u0003\u0000\u0004\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0029\u0000\u0000\u0000\u002a\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0024\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u0030\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0001\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0001\u0000\u0001\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u002d\u0000\u0000\u0000\u002a\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0028\u0000\u0000\u0000\u0003\u0000\u0001\u0000" +
   "\u0034\u0000\u0000\u0000\u0002\u0000\u0001\u0000" +
   "\u0072\u0065\u0070\u006f\u0000\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u000c\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u006d\u0061\u0072\u006b\u0000\u0000\u0000\u0000" +
   "\u0006\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0006\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" +
   "\u0000\u0000\u0000\u0000\u0000\u0000\u0000\u0000" + "");
}
}

