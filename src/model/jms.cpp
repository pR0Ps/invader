// SPDX-License-Identifier: GPL-3.0-only

#include <invader/model/jms.hpp>
#include <optional>

using namespace Invader::Model;
using namespace Invader::HEK;

static std::optional<std::string> read_word(const char *word, const char **word_end);
static std::optional<TagString> read_tagstring(const char *word, const char **word_end);
static long long read_int(const char *word, const char **word_end);
static double read_float(const char *word, const char **word_end);
static Quaternion<NativeEndian> read_quat(const char *word, const char **word_end);
static Point3D<NativeEndian> read_point3d(const char *word, const char **word_end);
static Vector3D<NativeEndian> read_vector3d(const char *word, const char **word_end);

void JMS::import_jms(const char *jms, const char **jms_end) {
    auto version = read_int(jms, &jms);
    if(version != 8200) {
        throw std::exception(); // invalid version
    }
    read_int(jms, &jms); // skip this
    
    // Go through each node
    auto node_count = read_int(jms, &jms);
    for(long long i = 0; i < node_count; i++) {
        this->nodes.emplace_back().import_jms(jms, &jms);
    }
    
    // Go through each material now
    auto material_count = read_int(jms, &jms);
    for(long long i = 0; i < material_count; i++) {
        this->materials.emplace_back().import_jms(jms, &jms);
    }
    
    // Go through each marker
    auto marker_count = read_int(jms, &jms);
    for(long long i = 0; i < marker_count; i++) {
        this->markers.emplace_back().import_jms(jms, &jms);
    }
    
    // Go through each marker
    auto region_count = read_int(jms, &jms);
    for(long long i = 0; i < region_count; i++) {
        this->regions.emplace_back().import_jms(jms, &jms);
    }
    
    // Go through each vertex
    auto vertex_count = read_int(jms, &jms);
    for(long long i = 0; i < vertex_count; i++) {
        this->vertices.emplace_back().import_jms(jms, &jms);
    }
    
    // Go through each triangle
    auto triangle_count = read_int(jms, &jms);
    for(long long i = 0; i < triangle_count; i++) {
        this->triangles.emplace_back().import_jms(jms, &jms);
    }

    if(jms_end) {
        *jms_end = jms;
    }
}

void JMS::Node::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, &jms).value();
    this->first_child_node = static_cast<Index>(read_int(jms, &jms));
    this->sibling_node_index = static_cast<Index>(read_int(jms, &jms));
    this->rotation = read_quat(jms, &jms);
    this->position = read_point3d(jms, jms_end);
}

void JMS::Material::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, &jms).value();
    this->tif_path = read_word(jms, jms_end).value();
}

void JMS::Marker::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, &jms).value();
    this->region = static_cast<Index>(read_int(jms, &jms));
    this->rotation = read_quat(jms, &jms);
    this->position = read_point3d(jms, jms_end);
}

void JMS::Region::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, jms_end).value();
}

void JMS::Vertex::import_jms(const char *jms, const char **jms_end) {
    this->node0 = static_cast<Index>(read_int(jms, &jms));
    this->position = read_point3d(jms, &jms);
    this->normal = read_vector3d(jms, &jms);
    this->node1 = static_cast<Index>(read_int(jms, &jms));
    this->node1_weight = read_float(jms, &jms);
    this->texture_coordinates = read_point3d(jms, jms_end);
}

void JMS::Triangle::import_jms(const char *jms, const char **jms_end) {
    this->region = static_cast<Index>(read_int(jms, &jms));
    this->shader = static_cast<Index>(read_int(jms, &jms));
    this->vertices[0] = static_cast<Index>(read_int(jms, &jms));
    this->vertices[1] = static_cast<Index>(read_int(jms, &jms));
    this->vertices[2] = static_cast<Index>(read_int(jms, jms_end));
}

// Read the word, treating tabs, newlines, and nulls as the end of the word.
static std::optional<std::string> read_word(const char *word, const char **word_end) {
    // Find the beginning
    while(true) {
        // Are we at the end? If so, return nullptr (no word present).
        char wc = *word;
        if(wc == 0) {
            return std::nullopt;
        }
        
        // Is this NOT a tab or newline? If so, break.
        if(wc == '\t' || wc == '\n' || wc == '\r') {
            break;
        }
        
        word++;
    }
    
    // Find the end
    const char *end_of_the_word = word;
    while(true) {
        // Are we at the end? If so, break.
        char wc = *end_of_the_word;
        if(wc == 0 || wc == '\t' || wc == '\n' || wc == '\r') {
            break;
        }
        
        end_of_the_word++;
    }
    
    // Set it!
    if(word_end) {
        *word_end = end_of_the_word;
    }
    
    return std::string(word, end_of_the_word);
}

static long long read_int(const char *word, const char **word_end) {
    return std::stoll(read_word(word, word_end).value());
}

static double read_float(const char *word, const char **word_end) {
    return std::stod(read_word(word, word_end).value());
}

static std::optional<TagString> read_tagstring(const char *word, const char **word_end) {
    auto str = read_word(word, &word);
    if(str.has_value() && str->size() < sizeof(TagString::string)) {
        if(word_end) {
            *word_end = word;
        }
        TagString ts = {};
        std::strncpy(ts.string, str->c_str(), str->size());
        return ts;
    }
    return std::nullopt;
}

static Quaternion<NativeEndian> read_quat(const char *word, const char **word_end) {
    Quaternion<NativeEndian> quat;
    quat.i = read_float(word, &word);
    quat.j = read_float(word, &word);
    quat.k = read_float(word, &word);
    quat.w = read_float(word, word_end); // the last one is word_end so we don't need to write a check for word_end - the read_word function does it for us
    return quat;
}

static Point3D<NativeEndian> read_point3d(const char *word, const char **word_end) {
    Point3D<NativeEndian> point;
    point.x = read_float(word, &word);
    point.y = read_float(word, &word);
    point.z = read_float(word, word_end); // the last one is word_end so we don't need to write a check for word_end - the read_word function does it for us
    return point;
}

static Vector3D<NativeEndian> read_vector3d(const char *word, const char **word_end) {
    return read_point3d(word, word_end); // this should implicitly convert
}
