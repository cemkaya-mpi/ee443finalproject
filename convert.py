# Import a 16-bit 48khz Signed Little Endian PCM Wav file, and create raw data. That is, remove the header.
# To test, execute 'aplay -f S16_LE -r 48000 /path/to/output.bin'  or its windows equivalent
import wave
obj = wave.open('input.wav','r')
print( "Number of channels",obj.getnchannels())
print ( "Sample width",obj.getsampwidth())
print ( "Frame rate.",obj.getframerate())
print ("Number of frames",obj.getnframes())
print ( "parameters:",obj.getparams())
# Pass "wb" to write a new file, or "ab" to append
with open("output.bin", "wb") as binary_file:
    num_bytes_written = 0
    # Write text or bytes to the file
    #binary_file.write("Write text by encoding\n".encode('utf8'))
    for i in range(obj.getnframes()):
    # for i in range(5):
        currentframe = obj.readframes(1)
        #print (currentframe);
        if i%2 ==0:
            num_bytes_written += binary_file.write(currentframe)
        #num_bytes_written += binary_file.write(b'\x00')
    print("Wrote %d bytes." % num_bytes_written)
obj.close()
