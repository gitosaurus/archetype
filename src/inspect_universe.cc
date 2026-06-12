#include <iostream>
#include <sstream>
#include <format>
#include <string>
#include <string_view>
#include <set>
#include <map>
#include <ranges>

#include "inspect_universe.hh"
#include "Universe.hh"
#include "Object.hh"
#include "Expression.hh"
#include "SystemObject.hh"

namespace archetype {

    // Escape a string for safe embedding in a quoted Turtle literal.
    // Mirrors escape_string in Value.cc; kept local to avoid a cross-TU dep.
    static std::string escape_literal(std::string_view s) {
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
    static std::string uri_encode(std::string_view s) {
        std::string encoded;
        for (auto ch : s) {
            if ((ch >= 'A' and ch <= 'Z')  or  (ch >= 'a' and ch <= 'z')  or
                (ch >= '0' and ch <= '9')  or
                 ch == '.'  or  ch == '_'  or  ch == '-'  or  ch == '~') {
                encoded += ch;
            } else {
                encoded += std::format("%{:02x}", static_cast<unsigned char>(ch));
            }
        }
        return encoded;
    }

    static std::string obj_name_for(int obj_id) {
        int identifier_id = Universe::instance().identifierForObject(obj_id);
        if (identifier_id < 0) {
            return "_:object_" + std::to_string(obj_id);
        }
        ObjectPtr obj = Universe::instance().getObject(obj_id);
        std::string prefix = obj->isPrototype() ? "type:" : "obj:";
        return prefix + Universe::instance().Identifiers.get(identifier_id);
    }

    // Join a phrase word list into a single space-separated string.
    static std::string join_phrase(const std::list<Value>& words) {
        std::ostringstream ss;
        for (auto ii = words.begin(); ii != words.end(); ++ii) {
            if (ii != words.begin()) ss << ' ';
            ss << (*ii)->getString();
        }
        return ss.str();
    }

    static std::string parser_mode_name(SystemParser::Mode_e mode) {
        switch (mode) {
            case SystemParser::VERBS: return "verbs";
            case SystemParser::NOUNS: return "nouns";
        }
        return "unknown";
    }

    static void write_rdf_prefixes(std::ostream& out) {
        out << "@base <http://derektjones.net/archetype/> .\n\n"
            << "@prefix rdf:       <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
            << "@prefix rdfs:      <http://www.w3.org/2000/01/rdf-schema#> .\n"
            << "@prefix xsd:       <http://www.w3.org/2001/XMLSchema#> .\n\n"
            << "@prefix archetype: <schema/> .\n"
            << "@prefix type:      <type/> .\n"
            << "@prefix obj:       <object/> .\n"
            << "@prefix attr:      <attr/> .\n"
            << "@prefix msg:       <msg/> .\n\n";
    }

    void write_parser_rdf(std::ostream& out, bool with_prefixes) {
        ObjectPtr systemObject = Universe::instance().getObject(Universe::SystemObjectId);
        SystemObject* system = dynamic_cast<SystemObject*>(systemObject.get());
        if (not system or not system->parser_) return;
        const auto& parser = *system->parser_;

        if (with_prefixes) write_rdf_prefixes(out);

        // -- Vocabulary: invert match tables so phrases are grouped by object --

        std::map<int, std::set<std::string>> phrases_by_object;
        for (const auto& [phrase, verb_id] : parser.verbMatches_) {
            phrases_by_object[verb_id].insert(join_phrase(phrase));
        }
        for (const auto& [phrase, noun_id] : parser.nounMatches_) {
            phrases_by_object[noun_id].insert(join_phrase(phrase));
        }

        // "Effective" phrases: noun phrases whose referent is currently in
        // scope (proximate).  Pre-baked here so consumers without SPARQL can
        // answer "what can the player type right now?" with a simple scan.
        std::map<int, std::set<std::string>> live_phrases_by_object;
        for (const auto& [phrase, noun_id] : parser.nounMatches_) {
            if (parser.proximate_.contains(noun_id)) {
                live_phrases_by_object[noun_id].insert(join_phrase(phrase));
            }
        }

        out << "# Vocabulary\n\n";
        for (const auto& [obj_id, phrases] : phrases_by_object) {
            out << obj_name_for(obj_id);
            bool first = true;
            for (const auto& p : phrases) {
                out << "\n    " << (first ? "" : "; ")
                    << "archetype:matchesPhrase " << escape_literal(p);
                first = false;
            }
            if (auto live = live_phrases_by_object.find(obj_id); live != live_phrases_by_object.end()) {
                for (const auto& p : live->second) {
                    out << "\n    ; archetype:matchesNow " << escape_literal(p);
                }
            }
            out << " .\n\n";
        }

        // -- Parser state --

        out << "# Parser state\n\n"
            << "archetype:parser a archetype:SystemParser"
            << "\n    ; archetype:mode " << escape_literal(parser_mode_name(parser.mode_));

        if (not parser.proximate_.empty()) {
            bool first = true;
            out << "\n    ; archetype:proximate ";
            for (int p_obj_id : parser.proximate_) {
                if (not first) out << ", ";
                out << obj_name_for(p_obj_id);
                first = false;
            }
        }

        if (not parser.playerCommand_.empty()) {
            out << "\n    ; archetype:playerCommand " << escape_literal(parser.playerCommand_);
        }
        if (not parser.normalized_.empty()) {
            out << "\n    ; archetype:normalized " << escape_literal(parser.normalized_);
        }
        if (not parser.parsedValues_.empty()) {
            bool first = true;
            out << "\n    ; archetype:parsedValue ";
            for (const auto& v : parser.parsedValues_) {
                if (not first) out << ", ";
                out << v->asRDF();
                first = false;
            }
        }

        out << " .\n\n";
    }

    void dump_universe_rdf(std::ostream& out, bool include_methods) {
        write_rdf_prefixes(out);

        for (int obj_id = 0; obj_id < Universe::instance().objectCount(); obj_id++) {
            ObjectPtr obj = Universe::instance().getObject(obj_id);
            if (not obj) continue;

            out << obj_name_for(obj_id);

            ObjectPtr parent = obj->parent();
            if (parent) {
                if (obj->isPrototype()) {
                    out << " a rdfs:Class\n"
                        << "    ; rdfs:subClassOf " << obj_name_for(parent->id());
                } else {
                    out << " a " << obj_name_for(parent->id());
                }
            } else if (obj_id == Universe::NullObjectId) {
                out << " a rdfs:Class";
            } else if (obj_id == Universe::SystemObjectId) {
                out << " a archetype:SystemObject";
            } else {
                out << " a " << obj_name_for(Universe::NullObjectId);
            }

            for (auto const& [attribute_id, expr] : obj->attributes_) {
                auto* val_expr = dynamic_cast<ValueExpression*>(expr.get());
                if (not val_expr) continue;
                ContextScope c;
                c->selfObject = obj;
                Value value = val_expr->evaluate();
                if (value->isDefined()) {
                    out << "\n    ; attr:" << Universe::instance().Identifiers.get(attribute_id)
                        << " " << value->asRDF();
                }
            }

            if (include_methods) {
                for (int message_id : obj->methods_ | std::views::keys) {
                    if (message_id == DefaultMethod) {
                        out << "\n    ; archetype:respondsTo archetype:default";
                    } else {
                        std::string message = Universe::instance().Messages.get(message_id);
                        out << "\n    ; archetype:respondsTo msg:" << uri_encode(message);
                    }
                }
            }

            out << " .\n\n";
        }

        if (include_methods) write_parser_rdf(out, /* with_prefixes = */ false);
    }

    void inspect_universe(Storage& in, std::ostream& out, bool include_methods) {
        in >> Universe::instance();
        dump_universe_rdf(out, include_methods);
    }
}
