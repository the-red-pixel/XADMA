package org.kucro3.XADMA;

import javax.annotation.Nonnegative;
import javax.annotation.Nonnull;

public final class XADMAHandle {
    XADMAHandle(long handle, String name, long size)
    {
        this.handle = handle;
        this.name = name;
        this.alive = true;
        this.size = size;
    }

    @Override
    public int hashCode()
    {
        return Long.hashCode(handle);
    }

    @Override
    public boolean equals(Object object)
    {
        if (!(object instanceof XADMAHandle))
            return false;

        XADMAHandle obj = (XADMAHandle) object;

        if (handle != obj.handle)
            return false;

        return true;
    }

    public @Nonnull String getName()
    {
        return name;
    }

    public @Nonnegative long getSize()
    {
        return size;
    }

    public boolean isAlive()
    {
        return alive;
    }

    void setAlive(boolean alive)
    {
        this.alive = alive;
    }

    long getHandle()
    {
        return handle;
    }

    private final String name;

    private final long handle;

    private final long size;

    private volatile boolean alive;
}