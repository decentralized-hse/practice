#include <gtest/gtest.h>

#include <cassert>
#include <stdexcept>
#include <tlv/io.hpp>

TEST(Tlv, TestWriteRecord) {
  tlv::RecordWriter writer;

  writer.WriteRecord('A', {'A'});
  writer.WriteRecord('b', {'B', 'B'});

  ll::Bytes buf = writer.Extract();
  ll::Bytes correct = {'a', 1, 'A', '2', 'B', 'B'};
  ASSERT_EQ(buf, correct);

  ll::Bytes c256(256, 'C');
  ASSERT_EQ(c256.size(), 256);

  writer = tlv::RecordWriter(std::move(buf));
  writer.WriteRecord('C', c256);

  buf = writer.Extract();
  ASSERT_EQ(buf.size(), correct.size() + 1 + 4 + c256.size());
  ASSERT_EQ(67, buf[correct.size()]);
  ASSERT_EQ(1, buf[correct.size() + 2]);

  tlv::RecordReader reader(std::move(buf));
  tlv::Record record = reader.ReadNext();
  ASSERT_EQ(record.header.literal, 'A');
  ASSERT_EQ(record.body, ll::Bytes{'A'});

  tlv::Record record2 = reader.ReadNext();
  ASSERT_EQ(record2.header.literal, '0');
  ASSERT_EQ(record2.body, (ll::Bytes{'B', 'B'}));

  tlv::Record record3 = reader.ReadNext();
  ASSERT_EQ(record3.header.literal, 'C');
  ASSERT_EQ(record3.body.size(), 256);
  ASSERT_EQ(record3.body, c256);

  ASSERT_THROW(reader.ReadNext(), std::runtime_error);
}
