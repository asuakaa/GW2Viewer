export module GW2Viewer.Utils;
import std;

export
{

template<typename T>
struct std::less<std::span<T>>
{
    bool operator()(std::span<T> const& a, std::span<T> const& b) const noexcept
    {
        return std::ranges::lexicographical_compare(a | std::views::take(24) | std::views::reverse, b | std::views::take(24) | std::views::reverse);
        //return std::ranges::lexicographical_compare(a | std::views::reverse, b | std::views::reverse);
    }
};

}

namespace proj
{
template<typename T> struct _addressof_for { static constexpr auto* get(T&& t) { return &t; } };
template<typename T> struct _addressof_for<std::reference_wrapper<T>> { static constexpr T* get(std::reference_wrapper<T> t) { return &t.get(); } };

export constexpr struct _addressof
{
    template<typename T> constexpr auto* operator()(T&& input) const { return _addressof_for<std::remove_reference_t<T>>::get(std::forward<std::remove_reference_t<T>>(input)); }
} addressof;
export constexpr struct _dereference { constexpr auto& operator()(auto&& input) const { return *input; } } dereference;
}
