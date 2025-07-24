from enum import Enum, unique

__all__ = [
    "ArtistType",
    "FormatterType",
    "LegendPosition",
    "LineType",
]


@unique
class GeneralEnum(Enum):
    def __eq__(self, other):
        if not isinstance(other, type(self)):
            raise TypeError(f"Cannot compare {type(self)} with {type(other).__name__}")
        return self.value == other.value

    def __ne__(self, other):
        return not self.__eq__(other)

    def __int__(self) -> int:
        return self.value


class ArtistType(GeneralEnum):
    LINE = 0
    CANDLE = 1


class FormatterType(GeneralEnum):
    LINEAR = 0
    TIME = 1
    NULL = 2


class LegendPosition(GeneralEnum):
    TOP_LEFT = 1
    TOP_RIGHT = 2
    BOTTOM_LEFT = 3
    BOTTOM_RIGHT = 4


class LineType(GeneralEnum):
    S_LINE = 0
    H_LINE = 1
    V_LINE = 2
