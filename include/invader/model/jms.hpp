// SPDX-License-Identifier: GPL-3.0-only

#ifndef INVADER__JMS__JMS_HPP
#define INVADER__JMS__JMS_HPP

#include "../tag/parser/parser.hpp"

#include <vector>

namespace Invader::Model {
    struct JMS {
        struct Node {
            HEK::TagString name;
            HEK::Index first_child_node;
            HEK::Index sibling_node_index;
            HEK::Quaternion<HEK::NativeEndian> rotation;
            HEK::Point3D<HEK::NativeEndian> position;
            
            /**
             * Read the JMS data
             * @param jms     data to read from
             * @param jms_end if non-null, this will be set to the end of all read data
             */
            void import_jms(const char *jms, const char **jms_end = nullptr);
            
            /**
             * Export into JMS data
             * @return jms data
             */
            std::string export_jms() const;
        };
        struct Material {
            HEK::TagString name;
            std::string tif_path;
            
            /**
             * Read the JMS data
             * @param jms     data to read from
             * @param jms_end if non-null, this will be set to the end of all read data
             */
            void import_jms(const char *jms, const char **jms_end = nullptr);
            
            /**
             * Export into JMS data
             * @return jms data
             */
            std::string export_jms() const;
        };
        struct Marker {
            HEK::TagString name;
            HEK::Index region;
            HEK::Quaternion<HEK::NativeEndian> rotation;
            HEK::Point3D<HEK::NativeEndian> position;
            float radius;
            
            /**
             * Read the JMS data
             * @param jms     data to read from
             * @param jms_end if non-null, this will be set to the end of all read data
             */
            void import_jms(const char *jms, const char **jms_end = nullptr);
            
            /**
             * Export into JMS data
             * @return jms data
             */
            std::string export_jms() const;
        };
        struct Region {
            HEK::TagString name;
            
            /**
            * Read the JMS data
            * @param jms     data to read from
            * @param jms_end if non-null, this will be set to the end of all read data
            */
            void import_jms(const char *jms, const char **jms_end = nullptr);
            
            /**
             * Export into JMS data
             * @return jms data
             */
            std::string export_jms() const;
        };
        struct Vertex {
            HEK::Index node0;
            HEK::Point3D<HEK::NativeEndian> position;
            HEK::Vector3D<HEK::NativeEndian> normal;
            HEK::Index node1;
            float node1_weight;
            HEK::Point3D<HEK::NativeEndian> texture_coordinates;
            
            /**
             * Read the JMS data
             * @param jms     data to read from
             * @param jms_end if non-null, this will be set to the end of all read data
             */
            void import_jms(const char *jms, const char **jms_end = nullptr);
            
            /**
             * Export into JMS data
             * @return jms data
             */
            std::string export_jms() const;
        };
        struct Triangle {
            HEK::Index region;
            HEK::Index shader;
            HEK::Index vertices[3];
            
            /**
             * Read the JMS data
             * @param jms     data to read from
             * @param jms_end if non-null, this will be set to the end of all read data
             */
            void import_jms(const char *jms, const char **jms_end = nullptr);
            
            /**
             * Export into JMS data
             * @return jms data
             */
            std::string export_jms() const;
        };
        
        std::vector<Node> nodes;
        std::vector<Material> materials;
        std::vector<Marker> markers;
        std::vector<Region> regions;
        std::vector<Vertex> vertices;
        std::vector<Triangle> triangles;
        
        /**
         * Read the JMS data
         * @param jms     data to read from
         * @param jms_end if non-null, this will be set to the end of all read data
         */
        void import_jms(const char *jms, const char **jms_end = nullptr);
        
        /**
         * Export the JMS data
         * @return jms data
         */
        std::string export_jms() const;
        
        /**
         * Write the JMS data to a model tag
         * @param tag model tag to write to
         */
        void write_tag([[maybe_unused]] Parser::Model &tag) const {
            throw std::exception();
        }
        
        /**
         * Write the JMS data to a gbxmodel tag
         * @param tag model tag to write to
         */
        void write_tag([[maybe_unused]] Parser::GBXModel &tag) const {
            throw std::exception();
        }
        
        /**
         * Write the JMS data to a physics tag
         * @param tag model tag to write to
         */
        void write_tag([[maybe_unused]] Parser::Physics &tag) const {
            throw std::exception();
        }
        
        /**
         * Write the JMS data to a model_collision_geometry tag
         * @param tag model tag to write to
         */
        void write_tag([[maybe_unused]] Parser::ModelCollisionGeometry &tag) const {
            throw std::exception();
        }
        
        /**
         * Write the JMS data to a scenario_structure_bsp tag
         * @param tag model tag to write to
         */
        void write_tag([[maybe_unused]] Parser::ScenarioStructureBSP &tag) const {
            throw std::exception();
        }
    };
}

#endif
