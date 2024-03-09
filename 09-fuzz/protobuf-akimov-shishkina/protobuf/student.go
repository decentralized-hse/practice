package protobuf

import (
	reflect "reflect"
	sync "sync"

	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type ProjectProto struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Repo string `protobuf:"bytes,1,opt,name=repo,proto3" json:"repo,omitempty"`
	Mark uint32 `protobuf:"varint,2,opt,name=mark,proto3" json:"mark,omitempty"`
}

func (x *ProjectProto) Reset() {
	*x = ProjectProto{}
	if protoimpl.UnsafeEnabled {
		mi := &file_student_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *ProjectProto) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*ProjectProto) ProtoMessage() {}

func (x *ProjectProto) ProtoReflect() protoreflect.Message {
	mi := &file_student_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use ProjectProto.ProtoReflect.Descriptor instead.
func (*ProjectProto) Descriptor() ([]byte, []int) {
	return file_student_proto_rawDescGZIP(), []int{0}
}

func (x *ProjectProto) GetRepo() string {
	if x != nil {
		return x.Repo
	}
	return ""
}

func (x *ProjectProto) GetMark() uint32 {
	if x != nil {
		return x.Mark
	}
	return 0
}

type StudentProto struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Name     string        `protobuf:"bytes,1,opt,name=name,proto3" json:"name,omitempty"`
	Login    string        `protobuf:"bytes,2,opt,name=login,proto3" json:"login,omitempty"`
	Group    string        `protobuf:"bytes,3,opt,name=group,proto3" json:"group,omitempty"`
	Practice []uint32      `protobuf:"varint,4,rep,packed,name=practice,proto3" json:"practice,omitempty"`
	Project  *ProjectProto `protobuf:"bytes,5,opt,name=project,proto3" json:"project,omitempty"`
	Mark     float32       `protobuf:"fixed32,6,opt,name=mark,proto3" json:"mark,omitempty"`
}

func (x *StudentProto) Reset() {
	*x = StudentProto{}
	if protoimpl.UnsafeEnabled {
		mi := &file_student_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *StudentProto) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*StudentProto) ProtoMessage() {}

func (x *StudentProto) ProtoReflect() protoreflect.Message {
	mi := &file_student_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use StudentProto.ProtoReflect.Descriptor instead.
func (*StudentProto) Descriptor() ([]byte, []int) {
	return file_student_proto_rawDescGZIP(), []int{1}
}

func (x *StudentProto) GetName() string {
	if x != nil {
		return x.Name
	}
	return ""
}

func (x *StudentProto) GetLogin() string {
	if x != nil {
		return x.Login
	}
	return ""
}

func (x *StudentProto) GetGroup() string {
	if x != nil {
		return x.Group
	}
	return ""
}

func (x *StudentProto) GetPractice() []uint32 {
	if x != nil {
		return x.Practice
	}
	return nil
}

func (x *StudentProto) GetProject() *ProjectProto {
	if x != nil {
		return x.Project
	}
	return nil
}

func (x *StudentProto) GetMark() float32 {
	if x != nil {
		return x.Mark
	}
	return 0
}

var File_student_proto protoreflect.FileDescriptor

var file_student_proto_rawDesc = []byte{
	0x0a, 0x0d, 0x73, 0x74, 0x75, 0x64, 0x65, 0x6e, 0x74, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12,
	0x08, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x22, 0x36, 0x0a, 0x0c, 0x50, 0x72, 0x6f,
	0x6a, 0x65, 0x63, 0x74, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x12, 0x0a, 0x04, 0x72, 0x65, 0x70,
	0x6f, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x04, 0x72, 0x65, 0x70, 0x6f, 0x12, 0x12, 0x0a,
	0x04, 0x6d, 0x61, 0x72, 0x6b, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0d, 0x52, 0x04, 0x6d, 0x61, 0x72,
	0x6b, 0x22, 0xb0, 0x01, 0x0a, 0x0c, 0x53, 0x74, 0x75, 0x64, 0x65, 0x6e, 0x74, 0x50, 0x72, 0x6f,
	0x74, 0x6f, 0x12, 0x12, 0x0a, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x12, 0x14, 0x0a, 0x05, 0x6c, 0x6f, 0x67, 0x69, 0x6e, 0x18,
	0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x05, 0x6c, 0x6f, 0x67, 0x69, 0x6e, 0x12, 0x14, 0x0a, 0x05,
	0x67, 0x72, 0x6f, 0x75, 0x70, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x05, 0x67, 0x72, 0x6f,
	0x75, 0x70, 0x12, 0x1a, 0x0a, 0x08, 0x70, 0x72, 0x61, 0x63, 0x74, 0x69, 0x63, 0x65, 0x18, 0x04,
	0x20, 0x03, 0x28, 0x0d, 0x52, 0x08, 0x70, 0x72, 0x61, 0x63, 0x74, 0x69, 0x63, 0x65, 0x12, 0x30,
	0x0a, 0x07, 0x70, 0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x18, 0x05, 0x20, 0x01, 0x28, 0x0b, 0x32,
	0x16, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2e, 0x50, 0x72, 0x6f, 0x6a, 0x65,
	0x63, 0x74, 0x50, 0x72, 0x6f, 0x74, 0x6f, 0x52, 0x07, 0x70, 0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74,
	0x12, 0x12, 0x0a, 0x04, 0x6d, 0x61, 0x72, 0x6b, 0x18, 0x06, 0x20, 0x01, 0x28, 0x02, 0x52, 0x04,
	0x6d, 0x61, 0x72, 0x6b, 0x42, 0x03, 0x5a, 0x01, 0x2e, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f,
	0x33,
}

var (
	file_student_proto_rawDescOnce sync.Once
	file_student_proto_rawDescData = file_student_proto_rawDesc
)

func file_student_proto_rawDescGZIP() []byte {
	file_student_proto_rawDescOnce.Do(func() {
		file_student_proto_rawDescData = protoimpl.X.CompressGZIP(file_student_proto_rawDescData)
	})
	return file_student_proto_rawDescData
}

var file_student_proto_msgTypes = make([]protoimpl.MessageInfo, 2)
var file_student_proto_goTypes = []interface{}{
	(*ProjectProto)(nil), // 0: protobuf.ProjectProto
	(*StudentProto)(nil), // 1: protobuf.StudentProto
}
var file_student_proto_depIdxs = []int32{
	0, // 0: protobuf.StudentProto.project:type_name -> protobuf.ProjectProto
	1, // [1:1] is the sub-list for method output_type
	1, // [1:1] is the sub-list for method input_type
	1, // [1:1] is the sub-list for extension type_name
	1, // [1:1] is the sub-list for extension extendee
	0, // [0:1] is the sub-list for field type_name
}

func init() { file_student_proto_init() }
func file_student_proto_init() {
	if File_student_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_student_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*ProjectProto); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_student_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*StudentProto); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_student_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   2,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_student_proto_goTypes,
		DependencyIndexes: file_student_proto_depIdxs,
		MessageInfos:      file_student_proto_msgTypes,
	}.Build()
	File_student_proto = out.File
	file_student_proto_rawDesc = nil
	file_student_proto_goTypes = nil
	file_student_proto_depIdxs = nil
}
