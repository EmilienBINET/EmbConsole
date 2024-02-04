#include "EmbConsole.hpp"
#include <memory>

namespace emb {
    namespace console {

        Options Option::operator+(Option const& a_other) const {
            Options opts{*this};
            opts = opts + a_other;
            return opts;
        }

        Options Option::operator+(Options const& a_other) const {
            Options opts{*this};
            opts = opts + a_other;
            return opts;
        }

        Options Options::fromMainArgs(int argc, char** argv) {
            return Options();
        }

        Options::Options() {
            m_vpOptions.push_back(std::make_shared<OptionStd>());
            m_vpOptions.push_back(std::make_shared<OptionFile>());
            m_vpOptions.push_back(std::make_shared<OptionUnixSocket>());
            m_vpOptions.push_back(std::make_shared<OptionSyslog>());
        }

        Options::Options(Option const& a_other) : Options() {
            *this = operator+(a_other);
        }

        Options Options::operator+(Option const& a_other) const {
            Options opts{*this};
            bool bFound{false};
            for(auto & elm : opts.m_vpOptions) {
                if(typeid(*elm) == typeid(a_other)) {
                    elm = a_other.copy();
                    bFound = true;
                }
            }
            if(!bFound) {
                opts.m_vpOptions.push_back(a_other.copy());
            }
            return opts;
        }

        Options Options::operator+(Options const& a_other) const {
            Options opts{*this};
            for(auto const& opt : a_other.m_vpOptions) {
                opts = opts + *opt;
            }
            return opts;
        }


#if 0
        class Option::OptionPrivate {
        };

        Option::Option() noexcept {
        }

        Option::Option(Option const&) noexcept {
        }

        Option::Option(Option const&&) noexcept {
        }

        Option::~Option() noexcept {
        }

        Option& Option::operator= (Option&) noexcept {
            return *this;
        }

        Option& Option::operator= (Option&&) noexcept {
            return *this;
        }
#endif
    } // console
} // emb