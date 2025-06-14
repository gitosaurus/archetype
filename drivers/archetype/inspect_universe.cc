#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <iterator>

#include "inspect_universe.hh"
#include "Universe.hh"
#include "Object.hh"
#include "SystemObject.hh"

namespace archetype {
        void inspect_universe(Storage& in, std::ostream& out) {
        in >> Universe::instance();
        out << "@base <http://derektjones.net/archetype/> .\n\n"
            << "@prefix rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
            << "@prefix rdfs:   <http://www.w3.org/2000/01/rdf-schema#> .\n"
            << "@prefix owl:    <http://www.w3.org/2002/07/owl#> .\n\n"
            << "@prefix system: <system/> .\n"
            << "@prefix type:   <type/> .\n"
            << "@prefix object: <object/> .\n\n\n"
            ;

        std::map<int, int> reverse_ids;
        auto object_name = [&](int obj_id) -> std::string {
            auto obj_name_iter = reverse_ids.find(obj_id);
            if (obj_name_iter == reverse_ids.end()) {
                return "_:object_" + std::to_string(obj_id);
            } else {
                ObjectPtr obj = Universe::instance().getObject(obj_id);
                std::string prefix = obj->isPrototype() ? "type:" : "object:";
                return prefix + Universe::instance().Identifiers.get(obj_name_iter->second);
            }
        };
        for (auto const& a : Universe::instance().ObjectIdentifiers) {
            reverse_ids.insert(std::make_pair(a.second, a.first));
        }
        for (int obj_id = 0; obj_id < Universe::instance().objectCount(); obj_id++) {
            out << object_name(obj_id);
            ObjectPtr obj = Universe::instance().getObject(obj_id);
            ObjectPtr parent = obj->parent();
            if (parent) {
                if (obj->isPrototype()) {
                    out << " a rdfs:Class\n"
                        << "    ; rdfs:subClassOf ";
                } else {
                    out << " a ";
                }
                auto parent_name_iter = reverse_ids.find(parent->id());
                assert(parent_name_iter != reverse_ids.end());
                out << "type:" << Universe::instance().Identifiers.get(parent_name_iter->second);
            } else if (obj_id == 0) {
                out << " a rdfs:Class";
            } else if (obj_id == 1) {
                out << " a system:object";
            }
            out << '\n';
            // This is the flat, not the inherited, view of the attributes and methods.
            // Only what is added by each type.
            for (auto const& attr : obj->attributes_) {
                // Archetype has an endearing (sigh) way of instantiating any attributes that have even been
                // observed with UNDEFINED.  This is one reason why saved games have greater size.
                const bool hide_undefined = true;
                auto value = dynamic_cast<const ValueExpression*>(attr.second.get());
                if (!hide_undefined  or  (value and value->evaluate()->isDefined())) {
                    out << "    ; prop:hasAttribute attr:" << Universe::instance().Identifiers.get(attr.first) << '\n';
                }
            }
            for (auto const& method : obj->methods_) {
                if (method.first == DefaultMethod) {
                    out << "    ; prop:hasMethodForMessage system:default" << '\n';
                } else {
                    std::string message = Universe::instance().Messages.get(method.first);
                    std::ostringstream encoded;
                    // Following RFC 3986 section 2.3 Unreserved Characters (January 2005),
                    // URI-encoding any characters that are not unreserved.
                    for (auto ch : message) {
                        if ((ch >= 'A' and ch <= 'Z')  or  (ch >= 'a' and ch <= 'z')  or
                             ch == '.'  or  ch == '_'  or  ch == '-'  or  ch == '~') {
                            encoded << ch;
                        } else {
                            encoded << '%' << std::setw(2) << std::setfill('0') << std::hex << int(ch);
                        }
                    }
                    out << "    ; prop:respondstoMessage <msg/" << encoded.str() << ">\n";
                }
            }
            out << ".\n\n";
        } // display all objects

        // Next, a situation report.  What does the parser know?
        ObjectPtr systemObject = Universe::instance().getObject(Universe::SystemObjectId);
        SystemObject* system_object = dynamic_cast<SystemObject*>(systemObject.get());
        assert(system_object != nullptr);
        // What verb and noun phrase match what objects?
        // We need to turn the parsing inside out here; the parsed matches are sorted
        // from longest to shortest, not arranged by object.

        auto phrase = [](const std::list<Value>& phr) -> std::string {
            std::ostringstream ss;
            ss << '"';
            for (auto ii = phr.begin(); ii != phr.end(); ++ii) {
                if (ii != phr.begin()) ss << ' ';
                ss << (*ii)->getString();
            }
            ss << '"';
            return ss.str();
        };
        std::map<int, std::set<std::string>> verb_objects;
        std::set<std::string> all_verb_phrases;
        for (const auto& vp : system_object->parser_->verbMatches_) {
            std::string phr = phrase(vp.first);
            all_verb_phrases.insert(phr);
            verb_objects[vp.second].insert(phr);
        }
        std::map<int, std::set<std::string>> noun_objects;
        std::set<std::string> all_noun_phrases;
        for (const auto& np : system_object->parser_->nounMatches_) {
            std::string phr = phrase(np.first);
            all_noun_phrases.insert(phr);
            noun_objects[np.second].insert(phr);
        }

        out << "VERBS:\n";
        for (const auto& vi : verb_objects) {
            out << object_name(vi.first) << " matched by ";
            std::copy(vi.second.begin(), vi.second.end(), std::ostream_iterator<std::string>(out, " "));
            out << '\n';
        }
        out << "NOUNS:\n";
        for (const auto& ni : noun_objects) {
            out << object_name(ni.first) << " matched by ";
            std::copy(ni.second.begin(), ni.second.end(), std::ostream_iterator<std::string>(out, " "));
            out << '\n';
        }
        out << "Proximate:";
        for (int p_obj_id : system_object->parser_->proximate_) {
            out << ' ' << object_name(p_obj_id);
        }
        out << '\n';

        // Put out the entire vocabulary list as JSON object of two arrays.
        // Crude, but we can count on it here, with a very simple structure.
        // The words will already have quotes around them.
        auto json_list = [](const std::set<std::string>& words) -> std::string {
            std::ostringstream ss;
            ss << '[';
            for (auto ii = words.begin(); ii != words.end(); ++ii) {
                if (ii != words.begin()) ss << ", ";
                ss << *ii;
            }
            ss << ']';
            return ss.str();
        };
        out << "{\"verbs\": " << json_list(all_verb_phrases) << ",\n";
        out << "\"nouns\": " << json_list(all_noun_phrases) << "}\n";
    } // inspect_universe
}