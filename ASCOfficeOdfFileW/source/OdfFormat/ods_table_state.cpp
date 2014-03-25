#include "precompiled_cpodf.h"
#include "logging.h"

#include "ods_table_state.h"
#include "ods_conversion_context.h"

#include "table.h"

#include "styles.h"

#include "style_table_properties.h"
#include "style_text_properties.h"
#include "style_paragraph_properties.h"
#include "style_graphic_properties.h"


namespace cpdoccore {
namespace odf {

ods_table_state::ods_table_state(ods_conversion_context & Context, office_element_ptr & elm): context_(Context)
{     
	office_table_ = elm; 

	current_table_row_ =0;
	current_table_column_ =0;

	current_column_level_ = 1;

	current_level_.push_back(office_table_);

}

void ods_table_state::set_table_name(std::wstring name)
{
	table_table* table = dynamic_cast<table_table*>(office_table_.get());
	if (table == NULL)return;

	table->table_table_attlist_.table_name_ = name;
}

void ods_table_state::set_table_hidden(bool Val)
{
	if (!office_table_style_)return;

	style_table_properties *table_properties = office_table_style_->style_content_.get_style_table_properties();
	if (table_properties == NULL)return;

	table_properties->table_format_properties_.table_display_ = !Val;

}

void ods_table_state::set_table_tab_color(_CP_OPT(color) & _color)
{
	if (!office_table_style_)return;

	style_table_properties *table_properties = office_table_style_->style_content_.get_style_table_properties();
	if (table_properties == NULL)return;

	table_properties->table_format_properties_.tableooo_tab_color_ = _color;
}

void ods_table_state::set_table_style(office_element_ptr & elm)
{	
	office_table_style_ = dynamic_cast<style*>(elm.get());

	if (!office_table_style_)return;

	table_table* table = dynamic_cast<table_table*>(office_table_.get());
	if (table == NULL)return;
	
	table->table_table_attlist_.table_style_name_ = office_table_style_->style_name_;
	//����� � �������� � �� ����� ����� ����� �������(�����)


}
void ods_table_state::start_group(office_element_ptr & elm)
{
	current_level_.back()->add_child_element(elm);
	current_level_.push_back(elm);
}

void ods_table_state::end_group()
{
	current_level_.pop_back();
}

void ods_table_state::add_column(office_element_ptr & elm, int repeated,office_element_ptr & style_elm)
{
	current_level_.back()->add_child_element(elm);

	std::wstring style_name;

	odf::style* style = dynamic_cast<odf::style*>(style_elm.get());
	if (style)style_name = style->style_name_;

	ods_element_state state = {elm, repeated,style_name, style_elm, current_column_level_};
  
	if (repeated > 10000)repeated = 1024;//????

	current_table_column_ += repeated;
    columns_.push_back(state);

	table_table_column* column = dynamic_cast<table_table_column*>(columns_.back().elm.get());
	if (column == NULL)return;

	if (style_name.length()>0) column->table_table_column_attlist_.table_style_name_ = style_ref(style_name);
	column->table_table_column_attlist_.table_number_columns_repeated_ = repeated;
	
}
void ods_table_state::set_column_default_cell_style(std::wstring & style_name)
{
	if (style_name.length() < 1)return;

	table_table_column* column = dynamic_cast<table_table_column*>(columns_.back().elm.get());
	if (column == NULL)return;

	column->table_table_column_attlist_.table_default_cell_style_name_ = style_ref(style_name);
}

void ods_table_state::set_column_width(int width)//cm, pt ???
{
	odf::style* style = dynamic_cast<odf::style*>(columns_.back().style_elm.get());
	if (!style)return;		

	style_table_column_properties * column_properties = style->style_content_.get_style_table_column_properties();
 	if (column_properties == NULL)return; //error ????

	column_properties->style_table_column_properties_attlist_.style_column_width_ = length(width/4.35,length::cm);
}
void ods_table_state::set_column_optimal_width(bool val)
{
	odf::style* style = dynamic_cast<odf::style*>(columns_.back().style_elm.get());
	if (!style)return;		

	style_table_column_properties * column_properties = style->style_content_.get_style_table_column_properties();
 	if (column_properties == NULL)return; //error ????

	column_properties->style_table_column_properties_attlist_.style_use_optimal_column_width_ = val;

}

void ods_table_state::set_table_dimension(std::wstring ref)
{
}

void ods_table_state::add_row(office_element_ptr & elm, int repeated,office_element_ptr & style_elm)
{
    current_table_column_ = 0; 
    current_table_row_+=repeated;

	current_level_.back()->add_child_element(elm);

	std::wstring style_name;

	odf::style* style = dynamic_cast<odf::style*>(style_elm.get());
	if (style)style_name = style->style_name_;

	ods_element_state state = {elm, repeated,style_name, style_elm};
  
    rows_.push_back(state);

	table_table_row* row = dynamic_cast<table_table_row*>(rows_.back().elm.get());
	if (row == NULL)return;

	if (style_name.length()>0) row->table_table_row_attlist_.table_style_name_ = style_ref(style_name);
	row->table_table_row_attlist_.table_number_rows_repeated_ = repeated;

	row_default_cell_style_name_= L"";

}
void ods_table_state::set_row_hidden(bool Val)
{
	table_table_row* row = dynamic_cast<table_table_row*>(rows_.back().elm.get());
	if (row == NULL)return;

	row->table_table_row_attlist_.table_visibility_ = table_visibility(table_visibility::Collapse);
}
void ods_table_state::set_row_optimal_height(bool val)
{
	odf::style* style = dynamic_cast<odf::style*>(rows_.back().style_elm.get());
	if (!style)return;		

	style_table_row_properties * row_properties = style->style_content_.get_style_table_row_properties();
 	if (row_properties == NULL)return; //error ????

	row_properties->style_table_row_properties_attlist_.style_use_optimal_row_height_ = val;
}

void ods_table_state::set_row_height(double height)
{
	odf::style* style = dynamic_cast<odf::style*>(rows_.back().style_elm.get());
	if (!style)return;		

	style_table_row_properties * row_properties = style->style_content_.get_style_table_row_properties();
 	if (row_properties == NULL)return; //error ????

	row_properties->style_table_row_properties_attlist_.style_row_height_ = length(height/4.85,length::cm);

}
int ods_table_state::current_column() const
{
    return current_table_column_;
}

int ods_table_state::current_row() const
{
    return current_table_row_;
}

void ods_table_state::set_row_default_cell_style(std::wstring & style_name)
{
	row_default_cell_style_name_= style_name;	//����������� ����� ���������� default-style (table_cell)!!!
	
	//if (style_name.length() < 1) return;

	//table_table_row* row = dynamic_cast<table_table_row*>(rows_.back().elm.get());
	//if (row == NULL)return;

	//row->table_table_row_attlist_.table_default_cell_style_name_ = style_ref(style_name);
}

office_element_ptr  & ods_table_state::current_row_element()
{
	if (rows_.size()>0)
		return rows_.back().elm;
	else
	{
	}
}
office_element_ptr  & ods_table_state::current_cell_element()
{
	if (cells_.size()>0)
		return cells_.back().elm;
	else
	{
	}
}
office_value_type::type oox_valuetype_2_odf(int oox_fmt)
{
	switch (oox_fmt)
	{
	case 2:
	case 3:
	case 4:
		return office_value_type::Float;
	case 9:
	case 10:
		return office_value_type::Percentage;
	case 14:
	case 15:
	case 16:
	case 17:
		return office_value_type::Date;
	case 18:
	case 19:
		return office_value_type::Time;
	case 49:
		return office_value_type::String;
	case 0://general
	default:
		return office_value_type::String;
	}
}
void ods_table_state::start_cell(office_element_ptr & elm, office_element_ptr & style_elm)
{
	current_row_element()->add_child_element(elm);
	
	table_table_cell* cell = dynamic_cast<table_table_cell*>(elm.get());
	if (cell == NULL)return;
	
	std::wstring style_name;

	odf::style* style = dynamic_cast<odf::style*>(style_elm.get());
	if (style)style_name = style->style_name_;
	else style_name = row_default_cell_style_name_;

	if (style_name.length() > 0) 
		cell->table_table_cell_attlist_.table_style_name_=	style_name;

	ods_cell_state state;
	state.elm = elm;  state.repeated = 1;  state.style_name = style_name; state.style_elm = style_elm;
	state.row=0;  state.col =0;

	current_table_column_++;
  
    cells_.push_back(state);
}

void ods_table_state::set_cell_format_value(int format_value)
{
	if (cells_.size() < 1)return;
	if (format_value == 0)return;	//general .. need detect

	table_table_cell* cell = dynamic_cast<table_table_cell*>(cells_.back().elm.get());
	if (cell == NULL)return;

	common_value_and_type_attlist cell_type;
	cell_type.office_value_type_ = office_value_type(oox_valuetype_2_odf(format_value));
	
	cell->table_table_cell_attlist_.common_value_and_type_attlist_ = cell_type;

}
void ods_table_state::set_cell_type(int type)
{
	if (cells_.size() < 1)return;

	table_table_cell* cell = dynamic_cast<table_table_cell*>(cells_.back().elm.get());
	if (cell == NULL)return;

	_CP_OPT(office_value_type) cell_type;
	switch (type)
	{
	case 0:		cell_type = office_value_type(office_value_type::Boolean);
		break;
	case 4:		cell_type = office_value_type(office_value_type::Float);
		break;
	case 3:
	case 5:		
	case 6:		
		cell_type = office_value_type(office_value_type::String);
		break;
	}
	if (cell_type)
	{
		if (!cell->table_table_cell_attlist_.common_value_and_type_attlist_)
		{
			cell->table_table_cell_attlist_.common_value_and_type_attlist_ = common_value_and_type_attlist();
		}
		cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_value_type_ = cell_type;
	}
}
void ods_table_state::set_merge_cells(int start_col, int start_row, int end_col, int end_row)
{
	//����� ����� ���������� (��������������) - ��������� ����� ��� ���������� �����
	//����� �������� ������� ������ � ������, � �� �������� ������ ��� ��� ������ ��� ���������� �������
	//todooo
	if (end_col - start_col < 0)return;
	if (end_row - start_row < 0)return;

	int spanned_cols = end_col - start_col + 1;
	int spanned_rows = end_row - start_row + 1;

	if (spanned_cols > 10000)spanned_cols = 1024;

	for (int i = 0; i < cells_.size(); i++)
	{
		if (cells_[i].row > end_row) break;

		if (/*cells_[i].row <= end_row && */cells_[i].row >= start_row)
		{
			if (/*(cells_[i].col <= end_col || cells_[i].col + cells_[i].repeated-1 <= end_col) 
				&&*/ (cells_[i].col >= start_col/* || cells_[i].col + cells_[i].repeated-1 >= start_col*/))
			{
				table_table_cell* cell = dynamic_cast<table_table_cell*>(cells_[i].elm.get());
				if (cell == NULL)return;

				cell->table_table_cell_attlist_extra_.table_number_columns_spanned_ = spanned_cols;
				cell->table_table_cell_attlist_extra_.table_number_rows_spanned_ = spanned_rows;

				break;
			}
		}
	}
}

void ods_table_state::set_cell_value(std::wstring & value)
{
	if (cells_.size() < 1)return;

	table_table_cell* cell = dynamic_cast<table_table_cell*>(cells_.back().elm.get());
	if (cell == NULL)return;

	if (!cell->table_table_cell_attlist_.common_value_and_type_attlist_)
	{
		cell->table_table_cell_attlist_.common_value_and_type_attlist_ = common_value_and_type_attlist();
		cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_value_type_ = office_value_type(office_value_type::Float);
		//��������... ���� ��� ����������� �����
	}
	
	if (cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_value_type_)
	{
		switch(cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_value_type_->get_type())
		{
		case office_value_type::String:
			cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_string_value_ = value;
			break;
		case office_value_type::Boolean:
			cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_boolean_value_ = value;
			break;
		case office_value_type::Date:
			cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_date_value_ = value;
			break;
		case office_value_type::Time:
			cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_time_value_ = value;
			break;
		case office_value_type::Currency:
			cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_currency_ = value;//������� ���������
		default:
			cell->table_table_cell_attlist_.common_value_and_type_attlist_->office_value_ = value;
		}
	}
	else
	{
		//general !!
	}
//  ������������ ��������
	//start_text()
	//start_paragraph();

	office_element_ptr text_elm;
	create_element(L"text", L"p",text_elm, &context_);

	cells_.back().elm->add_child_element(text_elm);

	//table_table_cell* cell = dynamic_cast<text_p*>(text_elm.get());
	//end_paragraph();
	//end_text();
}

void ods_table_state::set_cell_ref (std::wstring & ref, int col, int row)
{
	if (cells_.size() < 1)return;

	cells_.back().ref = ref;
	cells_.back().col = col;
	cells_.back().row = row;
}
void ods_table_state::end_cell()
{
}
void ods_table_state::add_default_cell(office_element_ptr &  elm, int repeated)
{
	current_row_element()->add_child_element(elm);
	
	table_table_cell* cell = dynamic_cast<table_table_cell*>(elm.get());
	if (cell == NULL)return;
	
	ods_cell_state state;
	state.elm = elm;  state.repeated = repeated;  /*state.style_name = style_name;*//* state.style_elm = style_elm;*/
	state.row=current_table_row_;  state.col =current_table_column_;
  
    cells_.push_back(state);
	
	current_table_column_+=repeated;

	cell->table_table_cell_attlist_.table_number_columns_repeated_ = repeated;

	if (row_default_cell_style_name_.length() > 0)
		cell->table_table_cell_attlist_.table_style_name_ = row_default_cell_style_name_;
}
}
}
