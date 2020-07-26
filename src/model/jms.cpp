// SPDX-License-Identifier: GPL-3.0-only

#include <invader/model/jms.hpp>
#include <optional>

using namespace Invader::Model;
using namespace Invader::HEK;

#define JMS_VERSION 8200

static std::optional<std::string> read_word(const char *word, const char **word_end);
static std::optional<TagString> read_tagstring(const char *word, const char **word_end);
static long long read_int(const char *word, const char **word_end);
static double read_float(const char *word, const char **word_end);
static Quaternion<NativeEndian> read_quat(const char *word, const char **word_end);
static Point3D<NativeEndian> read_point3d(const char *word, const char **word_end);
static Vector3D<NativeEndian> read_vector3d(const char *word, const char **word_end);

static std::string write_vector3d(const Vector3D<NativeEndian> &vec);
static std::string write_point3d(const Point3D<NativeEndian> &point);
static std::string write_quat(const Quaternion<NativeEndian> &quat);
static std::string write_float(double f);
static std::string write_int(long long i);
static std::string write_tagstring(const TagString &ts);

void JMS::import_jms(const char *jms, const char **jms_end) {
    auto version = read_int(jms, &jms);
    if(version != JMS_VERSION) {
        throw std::exception(); // invalid version
    }
    read_int(jms, &jms); // skip this
    
    // Go through each thing again
    auto read_array = [&jms](auto &array) {
        auto count = read_int(jms, &jms);
        for(long long i = 0; i < count; i++) {
            array.emplace_back().import_jms(jms, &jms);
        }
    };
    
    // Make it!
    read_array(this->nodes);
    read_array(this->materials);
    read_array(this->markers);
    read_array(this->regions);
    read_array(this->vertices);
    read_array(this->triangles);

    if(jms_end) {
        *jms_end = jms;
    }
}

std::string JMS::export_jms() const {
    std::string value = write_int(JMS_VERSION) + "\n" + write_int(0) + "\n";
    
    // Go through each thing again
    auto write_array = [&value](auto &array) {
        value += write_int(array.size()) + "\n";
        for(auto &i : array) {
            value += i.export_jms() + "\n";
        }
    };
    
    // Write it!
    write_array(this->nodes);
    write_array(this->materials);
    write_array(this->markers);
    write_array(this->regions);
    write_array(this->vertices);
    write_array(this->triangles);
    
    return value;
}

void JMS::Node::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, &jms).value();
    this->first_child_node = static_cast<Index>(read_int(jms, &jms));
    this->sibling_node_index = static_cast<Index>(read_int(jms, &jms));
    this->rotation = read_quat(jms, &jms);
    this->position = read_point3d(jms, jms_end);
}

std::string JMS::Node::export_jms() const {
    return write_tagstring(this->name) + "\n" + write_int(this->first_child_node) + "\n" + write_int(this->sibling_node_index) + "\n" + write_quat(this->rotation) + "\n" + write_point3d(this->position);
}

void JMS::Material::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, &jms).value();
    this->tif_path = read_word(jms, jms_end).value();
}

std::string JMS::Material::export_jms() const {
    return write_tagstring(this->name) + "\n" + this->tif_path;
}

void JMS::Marker::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, &jms).value();
    this->region = static_cast<Index>(read_int(jms, &jms));
    this->rotation = read_quat(jms, &jms);
    this->position = read_point3d(jms, jms_end);
}

std::string JMS::Marker::export_jms() const {
    return write_tagstring(this->name) + "\n" + write_int(this->region) + "\n" + write_quat(this->rotation) + "\n" + write_point3d(this->position) + "\n" + write_float(this->radius);
}

void JMS::Region::import_jms(const char *jms, const char **jms_end) {
    this->name = read_tagstring(jms, jms_end).value();
}

std::string JMS::Region::export_jms() const {
    return write_tagstring(this->name);
}

void JMS::Vertex::import_jms(const char *jms, const char **jms_end) {
    this->node0 = static_cast<Index>(read_int(jms, &jms));
    this->position = read_point3d(jms, &jms);
    this->normal = read_vector3d(jms, &jms);
    this->node1 = static_cast<Index>(read_int(jms, &jms));
    this->node1_weight = read_float(jms, &jms);
    this->texture_coordinates = read_point3d(jms, jms_end);
}

std::string JMS::Vertex::export_jms() const {
    return write_int(this->node0) + write_point3d(this->position) + "\n" + write_vector3d(this->normal) + "\n" + write_int(this->node1) + "\n" + write_float(this->node1_weight) + "\n" +  write_point3d(this->texture_coordinates);
}

void JMS::Triangle::import_jms(const char *jms, const char **jms_end) {
    this->region = static_cast<Index>(read_int(jms, &jms));
    this->shader = static_cast<Index>(read_int(jms, &jms));
    this->vertices[0] = static_cast<Index>(read_int(jms, &jms));
    this->vertices[1] = static_cast<Index>(read_int(jms, &jms));
    this->vertices[2] = static_cast<Index>(read_int(jms, jms_end));
}

std::string JMS::Triangle::export_jms() const {
    return write_int(this->region) + "\n" + write_int(this->shader) + "\n" + write_int(this->vertices[0]) + "\t" + write_int(this->vertices[1]) + "\t" + write_int(this->vertices[2]);
}

static std::string write_vector3d(const Vector3D<NativeEndian> &vec) {
    return write_float(vec.i.read()) + "\t" + write_float(vec.j.read()) + "\t" + write_float(vec.k.read());
}
static std::string write_point3d(const Point3D<NativeEndian> &point) {
    return write_vector3d(point);
}
static std::string write_quat(const Quaternion<NativeEndian> &quat) {
    return write_float(quat.i.read()) + "\t" + write_float(quat.j.read()) + "\t" + write_float(quat.k.read()) + "\t" + write_float(quat.w.read());
}
static std::string write_float(double f) {
    return std::to_string(f);
}
static std::string write_int(long long i) {
    return std::to_string(i);
}
static std::string write_tagstring(const TagString &ts) {
    return std::string(ts.string);
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
