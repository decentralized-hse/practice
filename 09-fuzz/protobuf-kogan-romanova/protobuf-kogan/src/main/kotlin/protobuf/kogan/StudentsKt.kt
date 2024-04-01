// Generated by the protocol buffer compiler. DO NOT EDIT!
// source: struct.proto

// Generated files should ignore deprecation warnings
@file:Suppress("DEPRECATION")
package protobuf.kogan;

@kotlin.jvm.JvmName("-initializestudents")
public inline fun students(block: protobuf.kogan.StudentsKt.Dsl.() -> kotlin.Unit): protobuf.kogan.Struct.Students =
  protobuf.kogan.StudentsKt.Dsl._create(protobuf.kogan.Struct.Students.newBuilder()).apply { block() }._build()
/**
 * Protobuf type `protobuf.kogan.Students`
 */
public object StudentsKt {
  @kotlin.OptIn(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)
  @com.google.protobuf.kotlin.ProtoDslMarker
  public class Dsl private constructor(
    private val _builder: protobuf.kogan.Struct.Students.Builder
  ) {
    public companion object {
      @kotlin.jvm.JvmSynthetic
      @kotlin.PublishedApi
      internal fun _create(builder: protobuf.kogan.Struct.Students.Builder): Dsl = Dsl(builder)
    }

    @kotlin.jvm.JvmSynthetic
    @kotlin.PublishedApi
    internal fun _build(): protobuf.kogan.Struct.Students = _builder.build()

    /**
     * An uninstantiable, behaviorless type to represent the field in
     * generics.
     */
    @kotlin.OptIn(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)
    public class StudentsProxy private constructor() : com.google.protobuf.kotlin.DslProxy()
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     */
     public val students: com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>
      @kotlin.jvm.JvmSynthetic
      get() = com.google.protobuf.kotlin.DslList(
        _builder.getStudentsList()
      )
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     * @param value The students to add.
     */
    @kotlin.jvm.JvmSynthetic
    @kotlin.jvm.JvmName("addStudents")
    public fun com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>.add(value: protobuf.kogan.Struct.Student) {
      _builder.addStudents(value)
    }
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     * @param value The students to add.
     */
    @kotlin.jvm.JvmSynthetic
    @kotlin.jvm.JvmName("plusAssignStudents")
    @Suppress("NOTHING_TO_INLINE")
    public inline operator fun com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>.plusAssign(value: protobuf.kogan.Struct.Student) {
      add(value)
    }
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     * @param values The students to add.
     */
    @kotlin.jvm.JvmSynthetic
    @kotlin.jvm.JvmName("addAllStudents")
    public fun com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>.addAll(values: kotlin.collections.Iterable<protobuf.kogan.Struct.Student>) {
      _builder.addAllStudents(values)
    }
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     * @param values The students to add.
     */
    @kotlin.jvm.JvmSynthetic
    @kotlin.jvm.JvmName("plusAssignAllStudents")
    @Suppress("NOTHING_TO_INLINE")
    public inline operator fun com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>.plusAssign(values: kotlin.collections.Iterable<protobuf.kogan.Struct.Student>) {
      addAll(values)
    }
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     * @param index The index to set the value at.
     * @param value The students to set.
     */
    @kotlin.jvm.JvmSynthetic
    @kotlin.jvm.JvmName("setStudents")
    public operator fun com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>.set(index: kotlin.Int, value: protobuf.kogan.Struct.Student) {
      _builder.setStudents(index, value)
    }
    /**
     * `repeated .protobuf.kogan.Student students = 1;`
     */
    @kotlin.jvm.JvmSynthetic
    @kotlin.jvm.JvmName("clearStudents")
    public fun com.google.protobuf.kotlin.DslList<protobuf.kogan.Struct.Student, StudentsProxy>.clear() {
      _builder.clearStudents()
    }

  }
}
@kotlin.jvm.JvmSynthetic
public inline fun protobuf.kogan.Struct.Students.copy(block: `protobuf.kogan`.StudentsKt.Dsl.() -> kotlin.Unit): protobuf.kogan.Struct.Students =
  `protobuf.kogan`.StudentsKt.Dsl._create(this.toBuilder()).apply { block() }._build()
