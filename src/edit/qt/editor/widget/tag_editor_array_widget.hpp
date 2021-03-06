// SPDX-License-Identifier: GPL-3.0-only

#ifndef INVADER__EDIT__QT__TAG_EDITOR_ARRAY_WIDGET_HPP
#define INVADER__EDIT__QT__TAG_EDITOR_ARRAY_WIDGET_HPP

#include "tag_editor_widget.hpp"

class QLineEdit;
class QComboBox;
class QPushButton;
class QVBoxLayout;
class QStandardItemModel;
class QSpinBox;

namespace Invader::EditQt {
    class TagEditorEditWidgetView;

    class TagEditorArrayWidget : public TagEditorWidget {
        Q_OBJECT

    public:
        /**
         * Instantiate an array widget
         * @param parent        parent widget
         * @param value         struct value
         * @param editor_window editor window
         */
        TagEditorArrayWidget(QWidget *parent, Parser::ParserStructValue *value, TagEditorWindow *editor_window);

        ~TagEditorArrayWidget() = default;

    public slots:
        void update_text();

    private slots:
        void perform_add();
        void perform_insert();
        void perform_delete();
        void perform_delete_all();
        void perform_duplicate();
        void perform_clear();
        void perform_shift_up();
        void perform_shift_down();
        void spinbox_update();

    private:
        class ToggleSpinBoxLabel;
        
        void regenerate_widget();
        void regenerate_enum();
        void set_buttons_enabled();
        void toggle_spin_box();

        int current_index() const noexcept;

        TagEditorEditWidgetView *tag_view_widget = nullptr;
        bool read_only = false;

        QComboBox *reflexive_index;
        QVBoxLayout *vbox_layout;

        QPushButton *add_button;
        QPushButton *insert_button;
        QPushButton *duplicate_button;
        QPushButton *delete_button;
        QPushButton *delete_all_button;
        QPushButton *clear_button;
        QPushButton *shift_up_button;
        QPushButton *shift_down_button;
        QSpinBox *spin_box;

        QStandardItemModel *item_model;
    };
}

#endif
