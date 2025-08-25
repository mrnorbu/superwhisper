"""
SuperWhisper — Enhanced Minimal floating UI with 3 states and global F9 hotkey.

Improvements:
- Fixed memory leaks and better resource management
- Enhanced Apple-like UI design with compact layout
- Better error handling and thread safety
- Configuration persistence
- Performance optimizations

States:
- Ready (green): Click or press F9 to start recording
- Recording (red): Capturing audio; press F9 or click to stop, or stops on silence
- Transcribing (amber): Processing; then copies and auto-pastes (macOS) into the active app

Dependencies:
    pip install faster-whisper sounddevice numpy scipy pyperclip pynput

macOS notes:
- To allow global hotkey listening and auto-paste (Cmd+V), grant Accessibility permission to Python:
  System Settings > Privacy & Security > Accessibility → add Python (or your packaged app).
"""

import tkinter as tk
import threading
import time
import json
import numpy as np
import sounddevice as sd
from scipy.io.wavfile import write
from faster_whisper import WhisperModel
import pyperclip
import subprocess
import platform
import os
from pathlib import Path

# Global hotkey support
try:
    from pynput import keyboard
    PYNPUT_AVAILABLE = True
except Exception:
    PYNPUT_AVAILABLE = False

# =========================
# Configuration & Settings
# =========================
class Settings:
    def __init__(self):
        self.config_path = Path.home() / ".superwhisper_config.json"
        self.defaults = {
            "model_size": "small",
            "silence_duration": 1.0,
            "max_duration": 30,
            "silence_threshold": 0.01,
            "sample_rate": 16000,
            "auto_paste": True,
            "window_x": 1200,
            "window_y": 120
        }
        self.load()
    
    def load(self):
        try:
            if self.config_path.exists():
                with open(self.config_path) as f:
                    data = json.load(f)
                    # Update with loaded data, keeping defaults for missing keys
                    for key, value in data.items():
                        if hasattr(self, key) or key in self.defaults:
                            setattr(self, key, value)
            # Set any missing attributes to defaults
            for key, value in self.defaults.items():
                if not hasattr(self, key):
                    setattr(self, key, value)
        except Exception:
            # Fallback to defaults on any error
            for key, value in self.defaults.items():
                setattr(self, key, value)
    
    def save(self):
        try:
            config_data = {}
            for key in self.defaults.keys():
                if hasattr(self, key):
                    config_data[key] = getattr(self, key)
            with open(self.config_path, 'w') as f:
                json.dump(config_data, f, indent=2)
        except Exception:
            pass

# Global settings instance
settings = Settings()

# UI Configuration - Enhanced Apple-like design
BG_COLOR = "#1C1C1E"           # iOS dark background
FG_TEXT = "#FFFFFF"            # Pure white text
FG_HINT = "#8E8E93"            # iOS secondary text

# Enhanced color scheme
COLOR_READY = "#30D158"        # iOS green
COLOR_RECORD = "#FF453A"       # iOS red
COLOR_TRANSCRIBE = "#FFD60A"   # iOS yellow
COLOR_ERROR = "#FF6B6B"        # Error state
SHADOW_COLOR = "#2C2C2E"       # Subtle shadow

# Font configuration - smaller sizes for compact layout
if platform.system() == "Darwin":
    UI_FONT = ("SF Pro Text", 10)
    UI_FONT_MEDIUM = ("SF Pro Text", 10, "normal")
    UI_FONT_SMALL = ("SF Pro Text", 9)
    ICON_FONT = ("SF Pro Display", 14, "bold")  # Reduced from 26
elif platform.system() == "Windows":
    UI_FONT = ("Segoe UI", 10)
    UI_FONT_MEDIUM = ("Segoe UI", 11, "normal")
    UI_FONT_SMALL = ("Segoe UI", 8)
    ICON_FONT = ("Segoe UI", 14444, "bold")  # Reduced from 24
else:
    UI_FONT = ("Inter", 10)
    UI_FONT_MEDIUM = ("Inter", 11, "normal")
    UI_FONT_SMALL = ("Inter", 8)
    ICON_FONT = ("Inter", 14, "bold")  # Reduced from 24

HOTKEY_LABEL = "F9"

# =========================
# Custom Exceptions
# =========================
class AudioError(Exception):
    pass

class TranscriptionError(Exception):
    pass

# =========================
# Compact Circular Button
# =========================
class CircularButton(tk.Canvas):
    """
    Compact circular button with smaller size and better proportions.
    """
    def __init__(self, parent, size=56, color=COLOR_READY, icon="•", **kwargs):  # Reduced from 80 to 56
        super().__init__(parent, width=size, height=size, 
                        highlightthickness=0, bg=parent["bg"], bd=0, **kwargs)
        self.size = size
        self.color = color
        self.is_hovering = False

        # Subtle drop shadow (proportionally smaller)
        self.shadow = self.create_oval(2, 2, size-1, size-1, 
                                     fill=SHADOW_COLOR, outline="")
        
        # Main circle
        self.circle = self.create_oval(1, 1, size-1, size-1, 
                                     fill=color, outline="", width=0)

        # Icon text
        self.icon_text = self.create_text(size//2, size//2, text=icon, 
                                         fill="white", font=ICON_FONT)

        # Event bindings
        self.action = None
        self.bind("<Button-1>", self._on_click)
        self.bind("<Enter>", self._on_enter)
        self.bind("<Leave>", self._on_leave)

    def set_action(self, func):
        self.action = func

    def set_state(self, color, icon):
        """Update color and icon with smooth transition."""
        self.color = color
        self.itemconfig(self.circle, fill=color)
        self.itemconfig(self.icon_text, text=icon)
        
        # Update hover state if needed
        if self.is_hovering:
            self._apply_hover_effect()

    def _on_click(self, event):
        if self.action:
            # Visual feedback on click
            self._animate_click()
            self.action()

    def _on_enter(self, event):
        self.is_hovering = True
        self._apply_hover_effect()

    def _on_leave(self, event):
        self.is_hovering = False
        self._remove_hover_effect()

    def _apply_hover_effect(self):
        """Apply subtle hover effect."""
        hover_color = self._adjust_brightness(self.color, 1.15)
        self.itemconfig(self.circle, fill=hover_color)

    def _remove_hover_effect(self):
        """Remove hover effect."""
        self.itemconfig(self.circle, fill=self.color)

    def _animate_click(self):
        """Brief visual feedback on click."""
        click_color = self._adjust_brightness(self.color, 0.85)
        self.itemconfig(self.circle, fill=click_color)
        self.after(100, lambda: self.itemconfig(self.circle, fill=self.color))

    @staticmethod
    def _adjust_brightness(hex_color, factor):
        """Adjust brightness of hex color by factor."""
        try:
            hex_color = hex_color.lstrip("#")
            r, g, b = int(hex_color[0:2], 16), int(hex_color[2:4], 16), int(hex_color[4:6], 16)
            
            if factor > 1:  # Brighten
                r = min(255, int(r + (255 - r) * (factor - 1)))
                g = min(255, int(g + (255 - g) * (factor - 1)))
                b = min(255, int(b + (255 - b) * (factor - 1)))
            else:  # Darken
                r = int(r * factor)
                g = int(g * factor)
                b = int(b * factor)
            
            return f"#{r:02x}{g:02x}{b:02x}"
        except:
            # Fallback to original color if parsing fails
            return hex_color

# =========================
# Main Application
# =========================
class SuperWhisperApp:
    """
    Enhanced SuperWhisper with compact layout and better text handling.
    """
    STATE_READY = "ready"
    STATE_RECORDING = "recording"
    STATE_TRANSCRIBING = "transcribing"
    STATE_ERROR = "error"

    def __init__(self, root):
        self.root = root
        self._setup_window()
        
        # Thread safety
        self._lock = threading.RLock()
        self._recording_thread = None
        self._transcription_thread = None
        
        # App state
        self.state = self.STATE_READY
        self.is_recording = False
        self.audio_buffer = []
        self.model = None
        self._temp_files = []
        
        # Global hotkey
        self._listener = None
        self._last_hotkey_time = 0.0
        
        # Build UI
        self._build_ui()
        
        # Setup cleanup and event handlers
        self.root.protocol("WM_DELETE_WINDOW", self.cleanup)
        self.root.bind("<Escape>", lambda e: self.cleanup())
        self.root.bind("<F9>", lambda e: self._on_hotkey())
        
        # Start services
        self._start_global_hotkey()
        
        # Initialize to ready state
        self._set_ready_state()

    def _setup_window(self):
        """Configure main window with compact dimensions."""
        self.root.title("SuperWhisper")
        self.root.configure(bg=BG_COLOR)
        self.root.overrideredirect(True)
        self.root.attributes("-topmost", True)
        self.root.resizable(False, False)
        
        # Smaller, more rectangular layout
        x, y = settings.window_x, settings.window_y
        self.root.geometry(f"180x120+{x}+{y}")  # Reduced from 240x180 to 180x120
        
        # Make window draggable
        self._drag_data = {"x": 0, "y": 0}

    def _build_ui(self):
        """Build compact UI with proper text wrapping."""
        # Main container with minimal padding
        container = tk.Frame(self.root, bg=BG_COLOR, padx=16, pady=12)  # Reduced padding
        container.pack(expand=True, fill="both")

        # Top section with button and status
        top_frame = tk.Frame(container, bg=BG_COLOR)
        top_frame.pack(fill="x", pady=(0, 8))

        # Circular button (smaller size)
        self.button = CircularButton(top_frame, size=56, color=COLOR_READY, icon="•")  # Reduced size
        self.button.set_action(self._on_button_click)
        self.button.pack(side="left")

        # Status text next to button
        status_frame = tk.Frame(top_frame, bg=BG_COLOR)
        status_frame.pack(side="left", fill="both", expand=True, padx=(12, 0))

        self.status = tk.Label(
            status_frame, 
            text="Loading…", 
            bg=BG_COLOR, 
            fg=FG_TEXT, 
            font=UI_FONT_MEDIUM,
            anchor="w",
            justify="left"
        )
        self.status.pack(fill="x")

        # Hint text in two lines below
        self.hint = tk.Label(
            container,
            text="Press F9 anywhere\nor click to record",  # Split into two lines
            bg=BG_COLOR,
            fg=FG_HINT,
            font=UI_FONT_SMALL,
            justify="center"
        )
        self.hint.pack(pady=(4, 0))

        # Make UI elements draggable
        for widget in [container, top_frame, status_frame, self.status, self.hint]:
            widget.bind("<Button-1>", self._start_drag)
            widget.bind("<B1-Motion>", self._on_drag)
            widget.bind("<ButtonRelease-1>", self._end_drag)

        # Load model asynchronously
        threading.Thread(target=self._load_model, daemon=True).start()

    def _load_model(self):
        """Load Whisper model with better error handling."""
        try:
            self._set_status("Loading…")
            
            # Try different model sizes based on available memory
            model_sizes = [settings.model_size, "small", "tiny", "base"]
            model_sizes = list(dict.fromkeys(model_sizes))  # Remove duplicates
            
            for size in model_sizes:
                try:
                    self.model = WhisperModel(
                        size, 
                        device="cpu", 
                        compute_type="int8",
                        download_root=Path.home() / ".cache" / "whisper"
                    )
                    settings.model_size = size  # Save working model size
                    settings.save()
                    break
                except Exception as e:
                    if size == model_sizes[-1]:  # Last attempt
                        raise e
                    continue
            
            self.root.after(0, self._set_ready_state)
            
        except Exception as e:
            error_msg = f"Model load failed"
            self.root.after(0, lambda: self._show_error(error_msg))

    def _set_ready_state(self):
        """Set app to ready state."""
        with self._lock:
            self.state = self.STATE_READY
            self.is_recording = False
            self.button.set_state(COLOR_READY, "•")
            self._set_status("Ready")
            self.hint.config(text="Press F9 anywhere\nor click to record")

    def _set_recording_state(self):
        """Set app to recording state and start recording."""
        with self._lock:
            if self.state != self.STATE_READY:
                return
                
            self.state = self.STATE_RECORDING
            self.is_recording = True
            self.audio_buffer.clear()  # Clear any previous buffer
            
            self.button.set_state(COLOR_RECORD, "■")
            self._set_status("Recording…")
            self.hint.config(text="Click or press F9\nto stop recording")
            
            # Start recording in separate thread
            self._recording_thread = threading.Thread(target=self._record_audio, daemon=True)
            self._recording_thread.start()

    def _set_transcribing_state(self):
        """Set app to transcribing state."""
        with self._lock:
            self.state = self.STATE_TRANSCRIBING
            self.is_recording = False
            
            self.button.set_state(COLOR_TRANSCRIBE, "…")
            self._set_status("Transcribing…")
            self.hint.config(text="Processing audio\nplease wait")
            
            # Start transcription in separate thread
            self._transcription_thread = threading.Thread(target=self._transcribe_audio, daemon=True)
            self._transcription_thread.start()

    def _show_error(self, message):
        """Show error state with auto-recovery."""
        with self._lock:
            self.state = self.STATE_ERROR
            self.is_recording = False
            
            self.button.set_state(COLOR_ERROR, "!")
            self._set_status("Error")
            self.hint.config(text="Will retry in\na moment…")
            
            # Auto-recover after 3 seconds
            self.root.after(3000, self._set_ready_state)

    def _on_button_click(self):
        """Handle button clicks based on current state."""
        if self.state == self.STATE_READY:
            if self.model is None:
                self._show_error("Model not ready")
                return
            self._set_recording_state()
        elif self.state == self.STATE_RECORDING:
            self.is_recording = False
        # Ignore clicks during transcribing or error states

    def _on_hotkey(self):
        """Handle global hotkey (F9) presses."""
        # Debounce to prevent rapid triggers
        now = time.time()
        if now - self._last_hotkey_time < 0.3:
            return
        self._last_hotkey_time = now
        
        if self.state == self.STATE_READY:
            if self.model is None:
                self._show_error("Model not ready")
                return
            self._set_recording_state()
        elif self.state == self.STATE_RECORDING:
            self.is_recording = False

    def _record_audio(self):
        """Record audio with improved error handling and memory management."""
        stream = None
        try:
            # Verify microphone access
            try:
                sd.check_input_settings(samplerate=settings.sample_rate, channels=1)
            except Exception as e:
                raise AudioError(f"Microphone access denied: {e}")

            # Start audio stream
            stream = sd.InputStream(
                samplerate=settings.sample_rate,
                channels=1,
                dtype="float32",
                blocksize=1024
            )
            stream.start()

            last_voice_time = time.time()
            start_time = time.time()
            max_buffer_chunks = settings.max_duration * settings.sample_rate // 1024

            while self.is_recording:
                try:
                    chunk, _ = stream.read(1024)
                    
                    # Voice activity detection
                    amplitude = float(np.abs(chunk).max())
                    if amplitude > settings.silence_threshold:
                        last_voice_time = time.time()

                    # Add to buffer with memory management
                    self.audio_buffer.append(chunk.copy())
                    
                    # Prevent excessive memory usage
                    if len(self.audio_buffer) > max_buffer_chunks:
                        # Keep only recent audio (sliding window)
                        self.audio_buffer = self.audio_buffer[-max_buffer_chunks//2:]

                    current_time = time.time()
                    
                    # Auto-stop conditions
                    if (current_time - last_voice_time) > settings.silence_duration:
                        break
                    if (current_time - start_time) > settings.max_duration:
                        break

                except Exception as e:
                    raise AudioError(f"Recording error: {e}")

        except AudioError as e:
            self.root.after(0, lambda: self._show_error("Mic error"))
            return
        except Exception as e:
            self.root.after(0, lambda: self._show_error("Recording failed"))
            return
        finally:
            # Always cleanup stream
            if stream:
                try:
                    stream.stop()
                    stream.close()
                except Exception:
                    pass

        # Move to transcription if we have audio
        if self.audio_buffer:
            self.root.after(0, self._set_transcribing_state)
        else:
            self.root.after(0, lambda: self._finish_with_message("No audio"))

    def _transcribe_audio(self):
        """Transcribe recorded audio with better error handling."""
        temp_file = None
        try:
            if not self.audio_buffer:
                self.root.after(0, lambda: self._finish_with_message("No audio"))
                return

            # Concatenate and validate audio
            audio_data = np.concatenate(self.audio_buffer, axis=0)
            if audio_data.size == 0 or np.max(np.abs(audio_data)) < 1e-4:
                self.root.after(0, lambda: self._finish_with_message("Too quiet"))
                return

            # Create temporary file
            temp_file = f"temp_recording_{int(time.time())}.wav"
            self._temp_files.append(temp_file)
            
            # Normalize and save audio
            audio_int16 = (np.clip(audio_data, -1.0, 1.0) * 32767).astype(np.int16)
            write(temp_file, settings.sample_rate, audio_int16)

            # Transcribe with Whisper
            segments, info = self.model.transcribe(
                temp_file, 
                vad_filter=True,
                word_timestamps=False
            )
            
            # Extract text
            text_segments = list(segments)
            if not text_segments:
                self.root.after(0, lambda: self._finish_with_message("No speech"))
                return

            transcribed_text = "".join(segment.text for segment in text_segments).strip()
            
            if transcribed_text:
                self._handle_transcription_result(transcribed_text)
                self.root.after(0, lambda: self._finish_with_message("Pasted"))
            else:
                self.root.after(0, lambda: self._finish_with_message("No speech"))

        except Exception as e:
            self.root.after(0, lambda: self._show_error("Transcribe failed"))
        finally:
            # Cleanup temporary files
            self._cleanup_temp_files()
            # Clear audio buffer to free memory
            self.audio_buffer.clear()

    def _handle_transcription_result(self, text):
        """Handle transcription result - copy to clipboard and optionally paste."""
        try:
            # Always copy to clipboard
            pyperclip.copy(text)
            
            # Auto-paste on macOS if enabled
            if settings.auto_paste and platform.system() == "Darwin":
                try:
                    subprocess.run([
                        "osascript", "-e",
                        'tell application "System Events" to keystroke "v" using command down'
                    ], check=False, timeout=2, capture_output=True)
                except Exception:
                    # If paste fails, text is still in clipboard
                    pass
                    
        except Exception as e:
            # Even if clipboard fails, don't crash
            self._show_error("Clipboard error")

    def _finish_with_message(self, message):
        """Show completion message and return to ready state."""
        self._set_status(message)
        self.root.after(1500, self._set_ready_state)

    def _cleanup_temp_files(self):
        """Clean up temporary audio files."""
        for temp_file in self._temp_files:
            try:
                if os.path.exists(temp_file):
                    os.remove(temp_file)
            except Exception:
                pass
        self._temp_files.clear()

    def _set_status(self, message):
        """Update status label."""
        if hasattr(self, 'status'):
            self.status.config(text=message)

    # Global hotkey management
    def _start_global_hotkey(self):
        """Start global hotkey listener."""
        if not PYNPUT_AVAILABLE:
            return

        def on_press(key):
            try:
                if key == keyboard.Key.f9:
                    now = time.time()
                    if now - self._last_hotkey_time < 0.25:
                        return
                    self.root.after(0, self._on_hotkey)
            except Exception:
                pass

        try:
            self._stop_global_hotkey()
            self._listener = keyboard.Listener(on_press=on_press)
            self._listener.daemon = True
            self._listener.start()
        except Exception:
            pass

    def _stop_global_hotkey(self):
        """Stop global hotkey listener."""
        if self._listener:
            try:
                self._listener.stop()
            except Exception:
                pass
            self._listener = None

    # Window dragging
    def _start_drag(self, event):
        """Start window drag operation."""
        self._drag_data["x"] = event.x
        self._drag_data["y"] = event.y

    def _on_drag(self, event):
        """Handle window dragging."""
        x = self.root.winfo_x() + (event.x - self._drag_data["x"])
        y = self.root.winfo_y() + (event.y - self._drag_data["y"])
        self.root.geometry(f"+{x}+{y}")

    def _end_drag(self, event):
        """End window drag and save position."""
        settings.window_x = self.root.winfo_x()
        settings.window_y = self.root.winfo_y()
        settings.save()

    def cleanup(self):
        """Clean up resources before closing."""
        # Stop recording
        self.is_recording = False
        
        # Stop global hotkey
        self._stop_global_hotkey()
        
        # Wait for threads to finish (with timeout)
        for thread in [self._recording_thread, self._transcription_thread]:
            if thread and thread.is_alive():
                thread.join(timeout=1.0)
        
        # Cleanup temp files
        self._cleanup_temp_files()
        
        # Clear model to free memory
        if self.model:
            del self.model
            self.model = None
        
        # Save settings
        settings.save()
        
        # Close window
        try:
            self.root.quit()
            self.root.destroy()
        except Exception:
            pass

# =========================
# Entry Point
# =========================
def main():
    """Main entry point."""
    root = tk.Tk()
    
    try:
        app = SuperWhisperApp(root)
        root.mainloop()
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"Application error: {e}")
    finally:
        try:
            root.quit()
        except Exception:
            pass

if __name__ == "__main__":
    main()

