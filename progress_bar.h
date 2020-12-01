/*
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License.
 */

#pragma once
#include <string_view>
#include <system_error>
#include <Windows.h>
#include <ShObjIdl.h>
#include "internal/base_native_control.h"
#include "internal/control_visuals.h"
#include "internal/interfaces.h"
#include "com.h"

namespace wl {

/// Native progress bar control.
///
/// Optionally reflects the progress in window taskbar with ITaskbarList3 COM interface.
///
///
/// #include <progress_bar.h>
/// @see @ref ex09
/// @see https://docs.microsoft.com/en-us/windows/win32/controls/progress-bar-control
/// @see https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-itaskbarlist3
class progress_bar final : public i_resizable_control, public i_resource_control {
private:
	_wli::base_native_control _base;

	com::lib _comLib{com::lib::init::LATER};
	com::ptr<ITaskbarList3> _taskbar;

public:
	/// Defines if the window taskbar button will reflect the progress bar state.
	enum class taskbar {
		/// Progress operations will be reflected in the window taskbar button.
		REFLECT,
		/// Ordinary progress bar, don't reflect progress status in window taskbar button.
		DONT_USE
	};

	/// Possible progress states.
	enum class state {
		/// No progress is being shown; taskbar button shows nothing.
		NONE,
		/// Normal progress operation, green color.
		NORMAL,
		/// Paused progress state, yellow color.
		PAUSED,
		/// Error progress state, red color.
		ERR,
		/// Side-to-side marquee, waiting state.
		INDETERMINATE
	};

	/// Constructor, explicitly defining the control ID.
	progress_bar(i_parent_window* parent, WORD ctrlId, taskbar reflectOnTaskbar = taskbar::DONT_USE)
		: _base{*parent, ctrlId}
	{
		if (reflectOnTaskbar == taskbar::REFLECT) {
			this->_comLib.initialize();
			this->_taskbar = com::co_create_instance<ITaskbarList3>(CLSID_TaskbarList);
		}
	}

	/// Constructor, with an auto-generated control ID.
	explicit progress_bar(i_parent_window* parent, taskbar reflectOnTaskbar = taskbar::DONT_USE)
		: progress_bar{parent, _wli::next_auto_ctrl_id(), reflectOnTaskbar} { }

	/// Move constructor.
	progress_bar(progress_bar&&) = default;

	/// Move assignment operator.
	progress_bar& operator=(progress_bar&&) = default;

	/// Calls CreateWindowEx().
	/// Range is 0-100 unless changed with set_max_val().
	/// @note This method is better suited when the control belongs to an
	/// ordinary window (not a dialog), thus being called during parent's
	/// WM_CREATE.
	/// @param pos Position within parent client area. Will be adjusted to match current system DPI.
	/// @param size Size of the control. Will be adjusted to match current system DPI.
	/// @param pbStyles Progress bar control styles. PBS_ constants.
	/// @param wStyles Window styles. WS_ constants.
	/// @param wExStyles Extended window styles. WS_EX_ constants.
	/// @see https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw
	/// @see https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-create
	/// @see https://docs.microsoft.com/en-us/windows/win32/controls/progress-bar-control-styles
	/// @see https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
	/// @see https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
	progress_bar& create(POINT pos, SIZE size = {140, 21},
		DWORD pbStyles = 0,
		DWORD wStyles = WS_CHILD | WS_VISIBLE,
		DWORD wExStyles = 0)
	{
		pos = _wli::control_visuals::multiply_dpi(pos);
		size = _wli::control_visuals::multiply_dpi(size);

		this->_base.create_window(PROGRESS_CLASSW, {}, pos, size, pbStyles | wStyles, wExStyles);
		return *this;
	}

	/// Calls CreateWindowEx(), with a standard height of 21 pixels.
	/// Range is 0-100 unless changed with set_max_val().
	/// @note This method is better suited when the control belongs to an
	/// ordinary window (not a dialog), thus being called during parent's
	/// WM_CREATE.
	/// @param pos Position within parent client area. Will be adjusted to match current system DPI.
	/// @param size Size of the control. Will be adjusted to match current system DPI.
	/// @param pbStyles Progress bar control styles. PBS_ constants.
	/// @param wStyles Window styles. WS_ constants.
	/// @param wExStyles Extended window styles. WS_EX_ constants.
	/// @see https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw
	/// @see https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-create
	/// @see https://docs.microsoft.com/en-us/windows/win32/controls/progress-bar-control-styles
	/// @see https://docs.microsoft.com/en-us/windows/win32/winmsg/window-styles
	/// @see https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
	progress_bar& create(POINT pos, UINT width,
		DWORD pbStyles = 0,
		DWORD wStyles = WS_CHILD | WS_VISIBLE,
		DWORD wExStyles = 0)
	{
		return this->create(pos, {static_cast<LONG>(width), 21},
			pbStyles, wStyles, wExStyles);
	}

private:
	void create_in_dialog() override { this->_base.create_window(); }

public:
	/// Exposes the subclassing handler methods. If at least one handle is added, the control will be subclassed.
	/// @warning If you call this method after the control is created, an exception will be thrown.
	/// @see https://docs.microsoft.com/en-us/windows/win32/controls/subclassing-overview
	[[nodiscard]] msg::wnd_events& on_subclass() { return this->_base.on_subclass(); }

	/// Returns the underlying HWND handle.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	/// Returns the control ID.
	[[nodiscard]] WORD ctrl_id() const noexcept override { return this->_base.ctrl_id(); }

	/// Defines a new maximum position for the progress bar.
	/// Minimum position is always zero.
	/// @see https://docs.microsoft.com/en-us/windows/win32/controls/pbm-setrange
	const progress_bar& set_max_pos(size_t maxPos) const noexcept
	{
		SendMessageW(this->hwnd(), PBM_SETRANGE, 0, MAKELPARAM(0, maxPos));
		return *this;
	}

	/// Retrieves the current maximum progress position.
	/// Min value is always zero.
	/// @see https://docs.microsoft.com/en-us/windows/win32/controls/pbm-getrange
	[[nodiscard]] size_t max_pos() const noexcept { return SendMessageW(this->hwnd(), PBM_GETRANGE, FALSE, 0); }

	/// Sets the state to state::NORMAL, and updates current progress position.
	const progress_bar& set_pos(size_t posVal) const
	{
		this->set_state(state::NORMAL);

		// https://stackoverflow.com/a/22471313/6923555
		size_t curMax = this->max_pos();
		if (posVal >= curMax) this->set_max_pos(curMax + 1);
		SendMessageW(this->hwnd(), PBM_SETPOS, static_cast<WPARAM>(posVal + 1), 0);
		SendMessageW(this->hwnd(), PBM_SETPOS, static_cast<WPARAM>(posVal), 0);
		if (posVal >= curMax) this->set_max_pos(curMax);

		if (this->_taskbar.empty()) return *this; // don't reflect in taskbar

		if (HRESULT hr = const_cast<progress_bar*>(this)->_taskbar->
				SetProgressValue(GetAncestor(this->hwnd(), GA_ROOTOWNER), posVal, this->max_pos());
			FAILED(hr))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"ITaskbarList3::SetProgressValue() failed in " __FUNCTION__ "().");
		}
		return *this;
	}

	/// Retrieves current progress position.
	/// @see https://docs.microsoft.com/en-us/windows/win32/controls/pbm-getpos
	[[nodiscard]] size_t pos() const noexcept { return SendMessageW(this->hwnd(), PBM_GETPOS, 0, 0); }

	/// Sets the current state of the progress bar.
	const progress_bar& set_state(state newState) const
	{
		this->_set_pb_marquee_style(newState == state::INDETERMINATE);

		switch (newState) {
		case state::NONE:
			SendMessageW(this->hwnd(), PBM_SETSTATE, PBST_NORMAL, 0);
			SendMessageW(this->hwnd(), PBM_SETPOS, 0, 0); // reset progress bar position
			this->_set_tb_state(TBPF_NOPROGRESS);
			break;

		case state::NORMAL:
			SendMessageW(this->hwnd(), PBM_SETSTATE, PBST_NORMAL, 0);
			this->_set_tb_state(TBPF_NORMAL);
			break;

		case state::PAUSED:
			SendMessageW(this->hwnd(), PBM_SETSTATE, PBST_PAUSED, 0);
			this->_set_tb_state(TBPF_PAUSED);
			break;

		case state::ERR:
			SendMessageW(this->hwnd(), PBM_SETSTATE, PBST_ERROR, 0);
			this->_set_tb_state(TBPF_ERROR);
			break;

		case state::INDETERMINATE:
			this->_set_tb_state(TBPF_INDETERMINATE);
		}
		return *this;
	}

private:
	void _set_pb_marquee_style(bool hasMarquee) const noexcept
	{
		// http://stackoverflow.com/a/23689663
		LONG_PTR curStyle = GetWindowLongPtrW(this->hwnd(), GWL_STYLE);

		if (hasMarquee && !(curStyle & PBS_MARQUEE)) {
			SetWindowLongPtrW(this->hwnd(), GWL_STYLE, curStyle | PBS_MARQUEE);
			SendMessageW(this->hwnd(), PBM_SETMARQUEE, TRUE, 0);
		}

		if (!hasMarquee && (curStyle & PBS_MARQUEE)) {
			SendMessageW(this->hwnd(), PBM_SETMARQUEE, FALSE, 0);
			SetWindowLongPtrW(this->hwnd(), GWL_STYLE, curStyle & ~PBS_MARQUEE);
		}
	}

	void _set_tb_state(TBPFLAG tbpfFlag) const
	{
		if (this->_taskbar.empty()) return; // don't reflect in taskbar

		if (HRESULT hr = const_cast<progress_bar*>(this)->_taskbar->
				SetProgressState(GetAncestor(this->hwnd(), GA_ROOTOWNER), tbpfFlag);
			FAILED(hr))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"ITaskbarList3::SetProgressState() failed in " __FUNCTION__ "().");
		}
	}
};

}//namespace wl