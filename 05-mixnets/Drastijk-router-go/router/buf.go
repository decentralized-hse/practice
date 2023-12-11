package router

import "io"

func ReadBuf(buf []byte, rdr io.Reader) ([]byte, error) {
	avail := cap(buf) - len(buf)
	if avail < 512 {
		l := 4096
		if len(buf) > 2048 {
			l = len(buf) * 2
		}
		newbuf := make([]byte, l)
		copy(newbuf[:], buf)
		buf = newbuf[:len(buf)]
	}
	idle := buf[len(buf):cap(buf)]
	n, err := rdr.Read(idle)
	if err != nil {
		return buf, nil
	}
	buf = buf[:len(buf)+n]
	return buf, nil
}

func WriteBuf(buf []byte, wrr io.Writer) ([]byte, error) {
	n, err := wrr.Write(buf)
	if err == nil {
		buf = buf[n:]
	}
	return buf, err
}
