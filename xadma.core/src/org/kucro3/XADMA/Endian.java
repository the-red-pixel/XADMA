package org.kucro3.XADMA;

public enum Endian {
    BIG_ENDIAN(0),
    LITTLE_ENDIAN(1);

    Endian(int value)
    {
        this.value = value;
    }

    int getValue()
    {
        return value;
    }

    private final int value;
}
