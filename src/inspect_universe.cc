#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <iterator>
#include <set>
#include <map>

#include "inspect_universe.hh"
#include "Universe.hh"
#include "Object.hh"
#include "Expression.hh"
#include "SystemObject.hh"

namespace archetype {

    // Escape a string for safe embedding in a quoted Turtle literal.
    // Mirrors escape_string in Value.cc; kept local to avoid a cross-TU dep.
    static std::string escape_literal(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 2);
        result += '"';
        for (char ch : s) {
            switch (ch) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                default:   result += ch;     break;
            }
        }
        result += '"';
        return result;
    }

    // URI-encode a message name following RFC 3986 section 2.3.
    static std::string uri_encode(const std::string& s) {
        std::ostringstream encoded;
        for (auto ch : s) {
            if ((ch >= 'A' and ch <= 'Z')  or  (ch >= 'a' and ch <= 'z')  or
                (ch >= '0' and ch <= '9')  or
                 ch == '.'  or  ch == '_'  or  ch == '-'  or  ch == '~') {
                encoded << ch;
            } else {
                encoded << '%' << std::setw(2) << std::setfill('0') << std::hex
                        << (static_cast<unsigned int>(static_cast<unsigned char>(ch)));
            }
        }
        return encoded.str();
    }

    void inspect_universe(Storage& in, std::ostream& out, bool include_methods) {
        in >> Universe::instance();

        // -- Build reverse lookup: object_id -> identifier_id --

        std::map<int, int> reverse_ids;
        for (auto const& a : Universe::instance().ObjectIdentifiers) {
            reverse_ids.insert(std::make_pair(a.second, a.first));
        }

        auto obj_name = [&](int obj_id) -> std::string {
            auto it = reverse_ids.find(obj_id);
            if (it == reverse_ids.end()) {
                return "_:object_" + std::to_string(obj_id);
            }
            ObjectPtr obj = Universe::instance().getObject(obj_id);
            std::string prefix = obj->isPrototype() ? "type:" : "obj:";
            return prefix + Universe::instance().Identifiers.get(it->second);
        };

        // -- Prefixes --

        out << "@base <http://derektjones.net/archetype/> .\n\n"
            << "@prefix rdf:       <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
            << "@prefix rdfs:      <http://www.w3.org/2000/01/rdf-schema#> .\n"
            << "@prefix xsd:       <http://www.w3.org/2001/XMLSchema#> .\n\n"
            << "@prefix archetype: <schema/> .\n"
            << "@prefix type:      <type/> .\n"
            << "@prefix obj:       <object/> .\n"
            << "@prefix attr:      <attr/> .\n"
            << "@prefix msg:       <msg/> .\n\n";

        // -- Objects --

        for (int obj_id = 0; obj_id < Universe::instance().objectCount(); obj_id++) {
            ObjectPtr obj = Universe::instance().getObject(obj_id);
            if (not obj) continue;

            out << obj_name(obj_id);

            // Type / inheritance
            ObjectPtr parent = obj->parent();
            if (parent) {
                if (obj->isPrototype()) {
                    out << " a rdfs:Class\n"
                        << "    ; rdfs:subClassOf " << obj_name(parent->id());
                } else {
                    out << " a " << obj_name(parent->id());
                }
            } else if (obj_id == Universe::NullObjectId) {
                out << " a rdfs:Class";
            } else if (obj_id == Universe::SystemObjectId) {
                out << " a archetype:SystemObject";
            } else {
                out << " a " << obj_name(Universe::NullObjectId);
            }

            // Attributes: flat view with evaluated values
            for (auto const& attr : obj->attributes_) {
                Value value = attr.second->evaluate();
                if (value->isDefined()) {
                    out << "\n    ; attr:" << Universe::instance().Identifiers.get(attr.first)
                        << " " << value->asRDF();
                }
            }

            // Methods: list which messages this object responds to
            if (include_methods) {
                for (auto const& method : obj->methods_) {
                    if (method.first == DefaultMethod) {
                        out << "\n    ; archetype:respondsTo archetype:default";
                    } else {
                        std::string message = Universe::instance().Messages.get(method.first);
                        out << "\n    ; archetype:respondsTo msg:" << uri_encode(message);
                    }
                }
            }

            out << " .\n\n";
        } // objects

        // -- Vocabulary: parser state --

        ObjectPtr systemObject = Universe::instance().getObject(Universe::SystemObjectId);
        SystemObject* system = dynamic_cast<SystemObject*>(systemObject.get());
        assert(system != nullptr);

        // Join a phrase word list into a single quoted string.
        auto phrase = [](const std::list<Value>& phr) -> std::string {
            std::ostringstream ss;
            for (auto ii = phr.begin(); ii != phr.end(); ++ii) {
                if (ii != phr.begin()) ss << ' ';
                ss << (*ii)->getString();
            }
            return ss.str();
        };

        // Invert the parser's match tables: group phrases by object.
        std::map<int, std::set<std::string>> verb_objects;
        for (const auto& vp : system->parser_->verbMatches_) {
            verb_objects[vp.second].insert(phrase(vp.first));
        }
        std::map<int, std::set<std::string>> noun_objects;
        for (const auto& np : system->parser_->nounMatches_) {
            noun_objects[np.second].insert(phrase(np.first));
        }

        // Emit vocabulary as RDF: each object's verb/noun phrases.
        out << "# Vocabulary\n\n";
        std::set<int> vocab_objects;
        for (const auto& vi : verb_objects) vocab_objects.insert(vi.first);
        for (const auto& ni : noun_objects) vocab_objects.insert(ni.first);

        for (int vo : vocab_objects) {
            out << obj_name(vo);
            bool first = true;
            auto vi = verb_objects.find(vo);
            if (vi != verb_objects.end()) {
                for (const auto& vp : vi->second) {
                    out << "\n    " << (first ? "" : "; ")
                        << "archetype:verbPhrase " << escape_literal(vp);
                    first = false;
                }
            }
            auto ni = noun_objects.find(vo);
            if (ni != noun_objects.end()) {
                for (const auto& np : ni->second) {
                    out << "\n    " << (first ? "" : "; ")
                        << "archetype:nounPhrase " << escape_literal(np);
                    first = false;
                }
            }
            out << " .\n\n";
        }

        // Proximate objects
        if (not system->parser_->proximate_.empty()) {
            out << "# Proximate objects (present in current context)\n\n"
                << "archetype:situation archetype:proximate";
            bool first = true;
            for (int p_obj_id : system->parser_->proximate_) {
                out << "\n    " << (first ? "" : ", ") << obj_name(p_obj_id);
                first = false;
            }
            out << " .\n\n";
        }
    } // inspect_universe
}
