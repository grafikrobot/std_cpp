#ifndef BFG_STD_CPP_OPT_HPP
#define BFG_STD_CPP_OPT_HPP

/*
Copyright 2017-2018 Rene Rivera
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
 */

#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "clara.hpp"

#include "cli.hpp"

namespace bfg_std_cpp {

class opt_type
{
    public:

    virtual auto index() const -> std::type_index = 0;
};

template <typename Opt>
class opt_base : public opt_type
{
    public:

    auto operator[]( std::string const &opt_name ) -> Opt &
    {
        (*clara_opt)[opt_name];
        return static_cast<Opt&>(*this);
    }

    auto operator()( std::string const &description ) -> Opt &
    {
        (*clara_opt)(description);
        return static_cast<Opt&>(*this);
    }

    auto operator>>(cli & c) -> class cli &
    {
        cli = &c;
        return (c |= *clara_opt);
    }

    auto index() const -> std::type_index override
    {
        return std::type_index(typeid(Opt));
    }

    protected:

    std::unique_ptr<clara::Opt> clara_opt;
    cli * cli = nullptr;

    template <typename F>
    void reset(F const & f, std::string const & hint)
    {
        this->clara_opt.reset(new clara::Opt(f, hint));
        this->clara_opt->optional();
    }

    template <typename F>
    void reset(F const & f)
    {
        this->clara_opt.reset(new clara::Opt(f));
        this->clara_opt->optional();
    }

    template <typename K, typename V>
    void process(V v, K * k = nullptr)
    {
        cli->process(v,static_cast<Opt*>(this));
    }
};

template <typename T, typename K, typename Enable = void>
class opt : public opt_base<opt<T, K, Enable>>
{
    public:

    opt() : value(new T) {}

    auto hint(std::string const & hint) -> opt &
    {
        this->reset([this](T v) -> void {
                *this->value = v;
                this->template process<K>(v);
            },
            hint
        );
        return *this;
    }

    auto operator*() -> T& { return *value; }

    private:

    std::shared_ptr<T> value;
};

template <typename T, typename K>
class opt<T, K, typename std::enable_if<std::is_same<bool, T>::value>::type>
    : public opt_base<opt<T, K, typename std::enable_if<std::is_same<bool, T>::value>::type>>
{
    public:

    opt() : value(std::make_shared<bool>(false)) {}

    auto flag() -> opt &
    {
        this->reset([this](T v) -> void {
                *this->value = v;
                this->template process<K>(v);
            }
        );
        return *this;
    }

    auto operator*() -> T& { return *value; }

    private:

    std::shared_ptr<bool> value;
};

namespace detail
{
    template <typename T>
    struct is_vector : std::false_type {};

    template <typename V>
    struct is_vector<std::vector<V>> : std::true_type {};
}

template <typename T, typename K>
class opt<T, K, typename std::enable_if<detail::is_vector<T>::value>::type>
    : public opt_base<opt<T, K, typename std::enable_if<detail::is_vector<T>::value>::type>>
{
    typedef typename T::value_type value_type;
    public:

    opt() : value(new T) {}

    auto hint(std::string const & hint) -> opt &
    {
        this->reset([this](value_type const & v) -> void {
                this->value->push_back(v);
                this->template process<K>(v);
            },
            hint
        );
        return *this;
    }

    auto operator*() -> T& { return *value; }

    private:

    std::shared_ptr<T> value;
};

template<typename K> using opt_flag = opt<bool, K>;
template<typename K> using opt_string = opt<std::string, K>;
template<typename K> using opt_strings = opt<std::vector<std::string>, K>;

}

#endif
