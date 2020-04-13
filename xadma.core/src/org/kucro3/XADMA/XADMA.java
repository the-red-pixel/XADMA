package org.kucro3.XADMA;

import com.theredpixelteam.redtea.util.Predication;
import com.theredpixelteam.redtea.util.ShouldNotReachHere;

import javax.annotation.Nonnegative;
import javax.annotation.Nonnull;
import java.io.*;

public final class XADMA {
    private XADMA()
    {
    }

    private static native int getSystemEndian0();

    public static @Nonnull Endian getSystemEndian()
    {
        int endian = getSystemEndian0();

        switch (endian)
        {
            case 0:
                return Endian.BIG_ENDIAN;

            case 1:
                return Endian.LITTLE_ENDIAN;

            default:
                throw new ShouldNotReachHere();
        }
    }

    private static native long querySize0(long handle);

    public static @Nonnull XADMAHandle require(@Nonnull String name)
    {
        Predication.requireNonNull(name);

        // lookup cache
        XADMAHandle handle;

        // require from native
        long nativeHandle = require0(name);

        if (nativeHandle == 0)
            throw new IllegalArgumentException("XADMA space not availble: " + name);

        handle = new XADMAHandle(nativeHandle, name, querySize0(nativeHandle));

        return handle;
    }

    private static native long require0(String name);

    public static @Nonnull XADMAHandle acquire(@Nonnull String name,
                                               @Nonnegative int expectedSize)
    {
        return acquire(name, expectedSize, false);
    }

    public static @Nonnull XADMAHandle acquire(@Nonnull String name,
                                               @Nonnegative int expectedSize,
                                               boolean forceAllocation)
    {
        Predication.requirePositive(expectedSize);
        Predication.requireNonNull(name);

        XADMAHandle handle;

        // acquire from native
        long nativeHandle = acquire0(name, expectedSize);

        long size = querySize0(nativeHandle);

        if (forceAllocation && size < expectedSize)
        {
            release0(nativeHandle);

            throw new IllegalArgumentException("XADMA space size non-satisfaction (Reallocation not supported).");
        }

        handle = new XADMAHandle(nativeHandle, name, size);

        return handle;
    }

    private static native long acquire0(String name, int expectedSize);

    public static @Nonnull XADMAHandle allocate(@Nonnull String name,
                                                @Nonnegative int size)
    {
        Predication.requirePositive(size);
        Predication.requireNonNull(name);

        XADMAHandle handle;

        // allocate from native
        long nativeHandle = allocate0(name, size);

        if (nativeHandle == 0)
            throw new IllegalArgumentException("XADMA space duplicated: " + name + " (allocation)");

        handle = new XADMAHandle(nativeHandle, name, querySize0(nativeHandle));

        return handle;
    }

    private static native long allocate0(String name, int size);

    @SuppressWarnings("SynchronizationOnLocalVariableOrMethodParameter")
    public static void release(@Nonnull XADMAHandle handle)
    {
        Predication.requireNonNull(handle);

        if (handle.isAlive())
            synchronized (handle)
            {
                if (handle.isAlive())
                {
                    release0(handle.getHandle());

                    handle.setAlive(false);
                }
            }
    }

    private static native int release0(long handle);

    // read & writes

    private static void checkHandleAndOffset(XADMAHandle handle, long offset, int size)
    {
        if (!handle.isAlive())
            throw new IllegalArgumentException("XADMA handle not alive");

        if (offset < 0 || (offset + size) > handle.getSize())
            throw new IndexOutOfBoundsException(offset + "(+" + size + ") out of " + handle.getSize());
    }

    public static void putByte(@Nonnull XADMAHandle handle,
                               @Nonnegative long offset,
                               byte value)
    {
        checkHandleAndOffset(handle, offset, SIZE_BYTE);

        putByte0(handle.getHandle(), offset, value);
    }

    private static native void putByte0(long handle, long offset, byte value);

    public static void putChar(@Nonnull XADMAHandle handle,
                               @Nonnegative long offset,
                               char value)
    {
        checkHandleAndOffset(handle, offset, SIZE_CHAR);

        putChar0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset, value);
    }

    public static void putChar(@Nonnull XADMAHandle handle,
                               @Nonnull Endian endian,
                               @Nonnegative long offset,
                               char value)
    {
        checkHandleAndOffset(handle, offset, SIZE_CHAR);

        putChar0(handle.getHandle(), endian.getValue(), offset, value);
    }

    private static native void putChar0(long handle, int endian, long offset, char value);

    public static void putShort(@Nonnull XADMAHandle handle,
                                @Nonnegative long offset,
                                short value)
    {
        checkHandleAndOffset(handle, offset, SIZE_SHORT);

        putShort0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset, value);
    }

    public static void putShort(@Nonnull XADMAHandle handle,
                                @Nonnull Endian endian,
                                @Nonnegative long offset,
                                short value)
    {
        checkHandleAndOffset(handle, offset, SIZE_SHORT);

        putShort0(handle.getHandle(), endian.getValue(), offset, value);
    }

    private static native void putShort0(long handle, int endian, long offset, short value);

    public static void putInt(@Nonnull XADMAHandle handle,
                              @Nonnegative long offset,
                              int value)
    {
        checkHandleAndOffset(handle, offset, SIZE_INT);

        putInt0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset, value);
    }

    public static void putInt(@Nonnull XADMAHandle handle,
                              @Nonnull Endian endian,
                              @Nonnegative long offset,
                              int value)
    {
        checkHandleAndOffset(handle, offset, SIZE_INT);

        putInt0(handle.getHandle(), endian.getValue(), offset, value);
    }

    private static native void putInt0(long handle, int endian, long offset, int value);

    public static void putLong(@Nonnull XADMAHandle handle,
                               @Nonnegative long offset,
                               long value)
    {
        checkHandleAndOffset(handle, offset, SIZE_LONG);

        putLong0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset, value);
    }

    public static void putLong(@Nonnull XADMAHandle handle,
                               @Nonnull Endian endian,
                               @Nonnegative long offset,
                               long value)
    {
        checkHandleAndOffset(handle, offset, SIZE_LONG);

        putLong0(handle.getHandle(), endian.getValue(), offset, value);
    }

    private static native void putLong0(long handle, int endian, long offset, long value);

    public static void putFloat(@Nonnull XADMAHandle handle,
                                @Nonnegative long offset,
                                float value)
    {
        checkHandleAndOffset(handle, offset, SIZE_FLOAT);

        putFloat0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset, value);
    }

    public static void putFloat(@Nonnull XADMAHandle handle,
                                @Nonnull Endian endian,
                                @Nonnegative long offset,
                                float value)
    {
        checkHandleAndOffset(handle, offset, SIZE_FLOAT);

        putFloat0(handle.getHandle(), endian.getValue(), offset, value);
    }

    private static native void putFloat0(long handle, int endian, long offset, float value);

    public static void putDouble(@Nonnull XADMAHandle handle,
                                 @Nonnegative long offset,
                                 double value)
    {
        checkHandleAndOffset(handle, offset, SIZE_DOUBLE);

        putDouble0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset, value);
    }

    public static void putDouble(@Nonnull XADMAHandle handle,
                                 @Nonnull Endian endian,
                                 @Nonnegative long offset,
                                 double value)
    {
        checkHandleAndOffset(handle, offset, SIZE_DOUBLE);

        putDouble0(handle.getHandle(), endian.getValue(), offset, value);
    }

    private static native void putDouble0(long handle, int endian, long offset, double value);

    public static byte getByte(@Nonnull XADMAHandle handle,
                               @Nonnegative long offset)
    {

        checkHandleAndOffset(handle, offset, SIZE_BYTE);

        return getByte0(handle.getHandle(), offset);
    }

    private static native byte getByte0(long handle, long offset);

    public static char getChar(@Nonnull XADMAHandle handle,
                               @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_CHAR);

        return getChar0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset);
    }

    public static char getChar(@Nonnull XADMAHandle handle,
                               @Nonnull Endian endian,
                               @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_CHAR);

        return getChar0(handle.getHandle(), endian.getValue(), offset);
    }

    private static native char getChar0(long handle, int endian, long offset);

    public static short getShort(@Nonnull XADMAHandle handle,
                                 @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_SHORT);

        return getShort0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset);
    }

    public static short getShort(@Nonnull XADMAHandle handle,
                                 @Nonnull Endian endian,
                                 @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_SHORT);

        return getShort0(handle.getHandle(), endian.getValue(), offset);
    }

    private static native short getShort0(long handle, int endian, long offset);

    public static int getInt(@Nonnull XADMAHandle handle,
                             @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_INT);

        return getInt0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset);
    }

    public static int getInt(@Nonnull XADMAHandle handle,
                             @Nonnull Endian endian,
                             @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_INT);

        return getInt0(handle.getHandle(), endian.getValue(), offset);
    }

    private static native int getInt0(long handle, int endian, long offset);

    public static long getLong(@Nonnull XADMAHandle handle,
                               @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_LONG);

        return getLong0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset);
    }

    public static long getLong(@Nonnull XADMAHandle handle,
                               @Nonnull Endian endian,
                               @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_LONG);

        return getLong0(handle.getHandle(), endian.getValue(), offset);
    }

    private static native long getLong0(long handle, int endian, long offset);

    public static float getFloat(@Nonnull XADMAHandle handle,
                                 @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_FLOAT);

        return getFloat0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset);
    }

    public static float getFloat(@Nonnull XADMAHandle handle,
                                 @Nonnull Endian endian,
                                 @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_FLOAT);

        return getFloat0(handle.getHandle(), endian.getValue(), offset);
    }

    private static native float getFloat0(long handle, int endian, long offset);

    public static double getDouble(@Nonnull XADMAHandle handle,
                                   @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_DOUBLE);

        return getDouble0(handle.getHandle(), DEFAULT_ENDIAN.getValue(), offset);
    }

    public static double getDouble(@Nonnull XADMAHandle handle,
                                   @Nonnull Endian endian,
                                   @Nonnegative long offset)
    {
        checkHandleAndOffset(handle, offset, SIZE_DOUBLE);

        return getDouble0(handle.getHandle(), endian.getValue(), offset);
    }

    private static native double getDouble0(long handle, int endian, long offset);

    private static File extractLibrary() throws IOException
    {
        String system = System.getProperty("os.name").toLowerCase();

        InputStream resourceStream;
        String resourceName;
        String tempFileName;

        if (system.contains("windows"))
        {
            String arch = System.getProperty("sun.arch.data.model");

            if ("64".equals(arch))
            {
                resourceName = LIB_WINDOWS_X64;
                tempFileName = LIB_WINDOWS_X64_TEMPFILE;
            }
            else
            {
                resourceName = LIB_WINDOWS_X86;
                tempFileName = LIB_WINDOWS_X86_TEMPFILE;
            }
        }
        else
            throw new UnsatisfiedLinkError("OS \"" + system + "\" not supported");

        resourceStream = XADMA.class.getResourceAsStream(resourceName);

        File file = new File(System.getProperty("java.io.tmpdir"),
                String.format(tempFileName, System.currentTimeMillis() + ""));

        if (file.exists())
            throw new IOException("Unable to extract library file: File duplication");

        if (!file.createNewFile())
            throw new IOException("Unable to extract library file: Failed to create file");

        BufferedOutputStream os = new BufferedOutputStream(new FileOutputStream(file));

        int b;
        while ((b = resourceStream.read()) != -1)
            os.write(b);

        os.flush();
        os.close();

        return file;
    }

    private static final String LIB_WINDOWS_X64 = "XADMA-native-win-x64.dll";

    private static final String LIB_WINDOWS_X64_TEMPFILE = "XADMA-native-win-x64-%s.dll";

    private static final String LIB_WINDOWS_X86 = "XADMA-native-win-x86.dll";

    private static final String LIB_WINDOWS_X86_TEMPFILE = "XADMA-native-win-x86-%s.dll";

    public static final Endian DEFAULT_ENDIAN;

    private static final int SIZE_BYTE = 1;

    private static final int SIZE_CHAR = 2;

    private static final int SIZE_SHORT = 2;

    private static final int SIZE_INT = 4;

    private static final int SIZE_LONG = 8;

    private static final int SIZE_FLOAT = 4;

    private static final int SIZE_DOUBLE = 8;

    static {
        try {
            System.load(extractLibrary().getAbsolutePath());
        } catch (IOException e) {
            throw new Error("Unable to load native library", e);
        }

        DEFAULT_ENDIAN = getSystemEndian();
    }
}
