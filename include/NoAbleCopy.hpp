#ifndef NOABLECOPY_HPP
#define NOABLECOPY_HPP
class NoAbleCopy
{
public:
    NoAbleCopy() = default;
    
    NoAbleCopy(const NoAbleCopy &) = delete;
    NoAbleCopy(const NoAbleCopy &&) = delete;
    NoAbleCopy &operator=(const NoAbleCopy &) = delete;
    NoAbleCopy &operator=(const NoAbleCopy &&) = delete;
};
#endif  // NOABLECOPY_HPP