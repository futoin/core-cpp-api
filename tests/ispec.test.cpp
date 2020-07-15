//-----------------------------------------------------------------------------
//   Copyright 2018 FutoIn Project
//   Copyright 2018 Andrey Galkin
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//-----------------------------------------------------------------------------

#include <boost/test/unit_test.hpp>

#include <futoin/ispec.hpp>
#include <futoin/ispectools.hpp>

namespace ftnst = futoin::spectools;
using futoin::IAsyncSteps;

class TestSpecTools : public futoin::ISpecTools
{
public:
    ~TestSpecTools() noexcept override = default;

    void add_spec(IAsyncSteps& asi, const futoin::string& spec) override
    {
        (void) asi;
        (void) spec;
    }

    void load_spec(
            futoin::IAsyncSteps& asi, const ftnst::IfaceVer& iface_ver) override
    {
        (void) asi;
        (void) iface_ver;
    }

    void load_spec(
            futoin::IAsyncSteps& asi, const futoin::string& iface_ver) override
    {
        load_spec(asi, parse_iface_ver(iface_ver));
    }

    ftnst::IfaceVer parse_iface_ver(const futoin::string& iface_ver) override
    {
        (void) iface_ver;
        return {};
    }

    const SpecDirs& spec_dirs() const override
    {
        return spec_dirs_;
    }

    void add_spec_dirs(const SpecDir& path) override
    {
        (void) path;
    }

    SpecDirs spec_dirs_;

    void on(const EventType& event, EventHandler& handler) noexcept override
    {
        (void) event;
        (void) handler;
    }
    void once(const EventType& event, EventHandler& handler) noexcept override
    {
        (void) event;
        (void) handler;
    }
    void off(const EventType& event, EventHandler& handler) noexcept override
    {
        (void) event;
        (void) handler;
    }
    void emit(const EventType& event) noexcept override
    {
        (void) event;
    }
    void emit(const EventType& event, NextArgs&& args) noexcept override
    {
        (void) event;
        (void) args;
    }
    void register_event_impl(
            EventType& event,
            TestCast test_cast,
            const NextArgs& model_args) noexcept override
    {
        (void) event;
        (void) test_cast;
        (void) model_args;
    }
};

class TestSpec : public futoin::ISpec
{
public:
    ~TestSpec() noexcept override = default;

    ftnst::ISpecTools& spectools() const noexcept override
    {
        return const_cast<TestSpecTools&>(spectools_);
    }

    const futoin::string& orig_json() const override
    {
        return orig_json_;
    }
    futoin::string build_json() const override
    {
        return {};
    }
    futoin::string build_name() const override
    {
        return {};
    }
    futoin::string build_filename() const override
    {
        return {};
    }

    const ftnst::Iface& iface() const noexcept override
    {
        return iface_;
    }
    const ftnst::Version& version() const noexcept override
    {
        return version_;
    }
    const ftnst::FTN3Rev& ftn3rev() const noexcept override
    {
        return ftn3rev_;
    }
    const ftnst::Types& types() const noexcept override
    {
        return types_;
    }
    const ftnst::Functions& funcs() const noexcept override
    {
        return funcs_;
    }
    const ftnst::IfaceVer& inherit() const noexcept override
    {
        return inherit_;
    }
    const ftnst::ImportList& imports() const noexcept override
    {
        return imports_;
    }
    const ftnst::Desc& desc() const noexcept override
    {
        return desc_;
    }
    const ftnst::RequirementList& requires() const noexcept override
    {
        return requires_;
    }

protected:
    TestSpecTools spectools_;
    futoin::string orig_json_;

    ftnst::Iface iface_;
    ftnst::Version version_;
    ftnst::FTN3Rev ftn3rev_;
    ftnst::Types types_;
    ftnst::Functions funcs_;
    ftnst::IfaceVer inherit_;
    ftnst::ImportList imports_;
    ftnst::Desc desc_;
    ftnst::RequirementList requires_;
};

BOOST_AUTO_TEST_SUITE(ispec) // NOLINT

BOOST_AUTO_TEST_CASE(instance) // NOLINT
{
    TestSpec tee;
}

BOOST_AUTO_TEST_CASE(ispectools) // NOLINT
{
    TestSpecTools tee;
}

BOOST_AUTO_TEST_CASE(functions) // NOLINT
{
    ftnst::Function func;

    ftnst::Functions funcs;
    funcs.emplace("f1", func);
    funcs.emplace("f2", ftnst::Function{});

    ftnst::Function::Param prm;
    prm.name = "prm";
    prm.type = "SomeType";
    prm.defaultVal = 123;
    prm.defaultVal = futoin::string{"123"};
    prm.desc = "desc";
    func.params.emplace_back(prm);
    func.params.emplace_back(ftnst::Function::Param{
            "prm2", "SomeType", futoin::any{futoin::string{"def"}}, "desc"});

    BOOST_CHECK(!func.is_single_result());
    func.result_single = "SomeType";
    BOOST_CHECK(func.is_single_result());

    ftnst::Function::ResultVar rv;
    rv.name = "rslt1";
    rv.type = "SomeType";
    rv.desc = "desc";
    func.result.emplace_back(rv);
    func.result.emplace_back(
            ftnst::Function::ResultVar{"rslt2", "SomeType", "desc"});

    BOOST_CHECK(!func.rawresult);
    BOOST_CHECK(!func.rawupload);
    BOOST_CHECK(!func.heavy);

    func.throws.emplace_back("SomeException");

    BOOST_CHECK_EQUAL(func.maxreqsize, 64 * 1024);
    BOOST_CHECK_EQUAL(func.maxrspsize, 64 * 1024);

    BOOST_CHECK(func.seclvl.empty());
    func.seclvl = "SomeLevel";

    func.desc = "";
}

BOOST_AUTO_TEST_CASE(types) // NOLINT
{
    ftnst::Type type;
    ftnst::Types types;

    types.emplace("t1", type);
    types.emplace("t2", type);
}

BOOST_AUTO_TEST_CASE(version) // NOLINT
{
    using ftnst::Version;

    BOOST_CHECK(Version(1, 1) == Version(1, 1));
    BOOST_CHECK_EQUAL(Version(1, 1) == Version(1, 2), false);
    BOOST_CHECK_EQUAL(Version(1, 1) == Version(2, 1), false);

    BOOST_CHECK(Version(1, 1) != Version(1, 2));
    BOOST_CHECK(Version(1, 1) != Version(2, 1));
    BOOST_CHECK_EQUAL(Version(1, 1) != Version(1, 1), false);

    BOOST_CHECK(Version(1, 1) < Version(1, 2));
    BOOST_CHECK(Version(1, 1) < Version(2, 1));
    BOOST_CHECK_EQUAL(Version(1, 1) < Version(1, 1), false);
    BOOST_CHECK_EQUAL(Version(1, 2) < Version(1, 1), false);
    BOOST_CHECK_EQUAL(Version(2, 1) < Version(1, 1), false);

    BOOST_CHECK(Version(1, 2) > Version(1, 1));
    BOOST_CHECK(Version(2, 1) > Version(1, 1));
    BOOST_CHECK_EQUAL(Version(1, 1) > Version(1, 1), false);
    BOOST_CHECK_EQUAL(Version(1, 1) > Version(1, 2), false);
    BOOST_CHECK_EQUAL(Version(1, 1) > Version(2, 1), false);

    BOOST_CHECK(Version(1, 1) <= Version(1, 1));
    BOOST_CHECK(Version(1, 1) <= Version(1, 2));
    BOOST_CHECK(Version(1, 1) <= Version(2, 1));
    BOOST_CHECK_EQUAL(Version(1, 2) <= Version(1, 1), false);
    BOOST_CHECK_EQUAL(Version(2, 1) <= Version(1, 1), false);

    BOOST_CHECK(Version(1, 1) >= Version(1, 1));
    BOOST_CHECK(Version(1, 2) >= Version(1, 1));
    BOOST_CHECK(Version(2, 1) >= Version(1, 1));
    BOOST_CHECK_EQUAL(Version(1, 1) >= Version(1, 2), false);
    BOOST_CHECK_EQUAL(Version(1, 1) >= Version(2, 1), false);
}

BOOST_AUTO_TEST_SUITE_END() // NOLINT
