//
// Copyright (c) 2020-2020 Martin Moene
//
// https://github.com/martinmoene/scope-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "scope-main.t.hpp"

#include <functional>
#include <iostream>

#if scope_CPP11_110
# define Amp(expr) (expr)
#else
# define Amp(expr) &(expr)
#endif

using namespace nonstd;

static bool is_called = false;

namespace on {

void exit()
{
    is_called = true;
    std::cout << "On exit.\n";
}

void fail()
{
    is_called = true;
    std::cout << "On fail.\n";
}

void success()
{
    is_called = true;
    std::cout << "On success.\n";
}
} // namespace on

CASE( "scope_exit: exit function is called at end of scope" )
{
    is_called = false;

    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_exit( on::exit );
#else
        scope_exit guard = make_scope_exit( on::exit );
#endif
    }

    EXPECT( is_called );
}

CASE( "scope_exit: exit function is called at end of scope (lambda)" )
{
#if scope_USE_POST_CPP98_VERSION
    is_called = false;

    // scope:
    {
        auto guard = make_scope_exit( [](){ is_called = true; } );
    }

    EXPECT( is_called );
#else
    EXPECT( !!"lambda is not available (no C++11)" );
#endif
}

CASE( "scope_exit: exit function is called when an exception occurs" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_exit( on::exit );
#else
        scope_exit guard = make_scope_exit( on::exit );
#endif
        throw std::exception();
    }
    catch(...) {}

    EXPECT( is_called );
}

CASE( "scope_exit: exit function is not called at end of scope when released" )
{
    is_called = false;

    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_exit( on::exit );
#else
        scope_exit guard = make_scope_exit( on::exit );
#endif
        guard.release();
    }

    EXPECT_NOT( is_called );
}

CASE( "scope_fail: exit function is called when an exception occurs" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_fail( on::fail );
#else
        scope_fail guard = make_scope_fail( on::fail );
#endif
        throw std::exception();
    }
    catch(...) {}

    EXPECT( is_called );
}

CASE( "scope_fail: exit function is not called when no exception occurs" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_fail( on::fail );
#else
        scope_fail guard = make_scope_fail( on::fail );
#endif
        // throw std::exception();
    }
    catch(...) {}

    EXPECT_NOT( is_called );
}

CASE( "scope_fail: exit function is not called when released" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_fail( on::fail );
#else
        scope_fail guard = make_scope_fail( on::fail );
#endif
        guard.release();

        throw std::exception();
    }
    catch(...) {}

    EXPECT_NOT( is_called );
}

CASE( "scope_success: exit function is called when no exception occurs" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_success( on::success );
#else
        scope_success guard = make_scope_success( on::success );
#endif
        // throw std::exception();
    }
    catch(...) {}

    EXPECT( is_called );
}

CASE( "scope_success: exit function is not called when an exception occurs" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_success( on::success );
#else
        scope_success guard = make_scope_success( on::success );
#endif
        throw std::exception();
    }
    catch(...) {}

    EXPECT_NOT( is_called );
}

CASE( "scope_success: exit function is not called when released" )
{
    is_called = false;

    try
    {
#if scope_USE_POST_CPP98_VERSION
        auto guard = make_scope_success( on::success );
#else
        scope_success guard = make_scope_success( on::success );
#endif
        guard.release();

        // throw std::exception();
    }
    catch(...) {}

    EXPECT_NOT( is_called );
}

// resource type to test unique_resource:

struct Resource
{
    enum { N = 20 };
    enum { free = 'f', acquired = 'a', closed = 'c', failed = 'x' };

    static size_t invalid()
    {
        return size_t(0);
    }

    static std::string& state()
    {
        static std::string state_( N, free );
        return state_;
    }

    static size_t & current()
    {
        static size_t index_ = 0;
        return index_;
    }

    static size_t next()
    {
        return ++current();
    }

    static size_t open ( bool success )
    {
        std::cout << "Resource open: " << (success ? "success: ":"failure: [no close]\n");
        const size_t i = next();
        state()[i] = success ? acquired : failed;
        return success ? i : invalid();
    }

    static void close( size_t i )
    {
        state()[i] = closed;
        std::cout << "close, state(): '" << state()[i] << "'\n";
    }

    static bool is_acquired( size_t index = current() )
    {
        return state()[index] == acquired;
    }

    static bool is_deleted( size_t index = current() )
    {
        return state()[index] == closed;
    }
};

CASE( "unique_resource: a successfully acquired resource is deleted" )
{
    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
# if !scope_BETWEEN(scope_COMPILER_MSVC_VERSION, 100, 120)
        // Use value initialization once here, direct initialization furtheron:

        auto cr{ make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        )};
# else
        auto cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
# endif
#else
        unique_resource<size_t, void(*)(size_t)> cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#endif
        EXPECT( Resource::is_acquired() );
    }

    EXPECT( Resource::is_deleted() );
}

CASE( "unique_resource: an unsuccessfully acquired resource is not deleted" )
{
    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            Resource::open( false ), Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr( make_unique_resource_checked(
            Resource::open( false ), Resource::invalid(), Amp(Resource::close)
        ));
#endif
        EXPECT_NOT( Resource::is_acquired() );
    }

    EXPECT_NOT( Resource::is_deleted() );
}

CASE( "unique_resource: op=() replaces the managed resouce and the deleter with the give one's" " [move-assignment]" )
{
    size_t r1;
    size_t r2;

    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto cr1( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr1( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#endif
        r1 = cr1.get();

        cr1 = make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        );

        r2 = cr1.get();

        EXPECT    ( Resource::is_deleted( r1 ) );
        EXPECT_NOT( Resource::is_deleted( r2 ) );
    }

    EXPECT( Resource::is_deleted( r2 ) );
}

CASE( "unique_resource: reset() executes deleter" )
{
    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#endif
        cr.reset();

        EXPECT( Resource::is_deleted() );
    }

    EXPECT( Resource::is_deleted() );
}

CASE( "unique_resource: reset(resource) deletes original resource and replaces it with the given one" )
{
    size_t r1;
    size_t r2;

    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto cr1( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr1( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#endif
        r1 = cr1.get();
        r2 = Resource::open( true );

        cr1.reset( r2 );

        EXPECT    ( Resource::is_deleted( r1 ) );
        EXPECT_NOT( Resource::is_deleted( r2 ) );
    }

    EXPECT( Resource::is_deleted( r2 ) );
}

CASE( "unique_resource: release() releases the ownership and prevents execution of deleter" )
{
    // scope:
    {
#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#endif
        cr.release();

        EXPECT_NOT( Resource::is_deleted() );
    }

    EXPECT_NOT( Resource::is_deleted() );
}

CASE( "unique_resource: get() provides the underlying resource handle" )
{
    size_t r = Resource::open( true );

#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            r, Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr( make_unique_resource_checked(
            r, Resource::invalid(), Amp(Resource::close)
        ));
#endif

    EXPECT( cr.get() == r );
}

CASE( "unique_resource: get_deleter() provides the deleter used for disposing of the managed resource" )
{
#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#else
        unique_resource<size_t, void(*)(size_t)> cr( make_unique_resource_checked(
            Resource::open( true ), Resource::invalid(), Amp(Resource::close)
        ));
#endif

    // note: lest does not support op=( T (*)(...), T (*)(...) ):

    EXPECT(( cr.get_deleter() == Amp(Resource::close) ));
}

CASE( "unique_resource: op*() provides the pointee if the resource handle is a pointer" )
{
    struct no { static void op( int const * ){} };

    int i = 77;

#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            &i, nullptr, Amp(no::op)
        ));
#else
        unique_resource<int *, void(*)(int const *)> cr( make_unique_resource_checked(
            &i, (int *)0, Amp(no::op)
        ));
#endif

    EXPECT( *cr == 77 );
}

struct S { int i; } s = { 77 };

CASE( "unique_resource: op->() provides the pointee if the resource handle is a pointer " )
{
    struct no { static void op( S const * ){} };

#if scope_USE_POST_CPP98_VERSION
        auto cr( make_unique_resource_checked(
            &s, nullptr, Amp(no::op)
        ));
#else
        unique_resource<S *, void(*)(S const *)> cr( make_unique_resource_checked(
            &s, (S *)0, Amp(no::op)
        ));
#endif

    EXPECT( cr->i == 77 );
}

CASE( "unique_resource: " "[move-construction][on-deleter-throws]")
{
}
