# automatically generated by the FlatBuffers compiler, do not modify

# namespace: schema

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Project(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Project()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsProject(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Project
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Project
    def Repo(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # Project
    def Mark(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, o + self._tab.Pos)
        return 0

def ProjectStart(builder): builder.StartObject(2)
def Start(builder):
    return ProjectStart(builder)
def ProjectAddRepo(builder, repo): builder.PrependUOffsetTRelativeSlot(0, flatbuffers.number_types.UOffsetTFlags.py_type(repo), 0)
def AddRepo(builder, repo):
    return ProjectAddRepo(builder, repo)
def ProjectAddMark(builder, mark): builder.PrependUint8Slot(1, mark, 0)
def AddMark(builder, mark):
    return ProjectAddMark(builder, mark)
def ProjectEnd(builder): return builder.EndObject()
def End(builder):
    return ProjectEnd(builder)