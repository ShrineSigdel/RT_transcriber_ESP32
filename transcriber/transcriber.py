"""
transcriber.py
==============
Wraps the Groq Whisper API call in a clean, reusable class.

Why isolated?
  All the messy details (temp files, error handling, API quirks) live here.
  The rest of the app just calls `transcriber.transcribe(audio_bytes)` and
  gets back a plain string — no Groq imports needed anywhere else.

Whisper expects an audio *file*, not raw bytes, so we write a temporary
.webm file, make the API call, then delete it immediately.  This is the
simplest cross-platform approach that works with the Groq SDK.
"""

import os
import tempfile
from groq import Groq
from config import settings


class AudioTranscriber:
    """Sends audio chunks to Groq Whisper and returns transcribed text."""

    def __init__(self):
        if not settings.groq_api_key:
            # Raise early so the developer sees this immediately on startup,
            # rather than getting a confusing 401 error on the first request.
            raise ValueError(
                "GROQ_API_KEY is not set.\n"
                "  export GROQ_API_KEY='gsk_...'\n"
                "  Get a free key at https://console.groq.com/keys"
            )
        self._client = Groq(api_key=settings.groq_api_key)

    def transcribe(self, audio_bytes: bytes, mime_type: str = "audio/webm") -> str:
        """
        Transcribe a chunk of audio and return the text.

        Parameters
        ----------
        audio_bytes : bytes
            Raw audio data captured by the browser's MediaRecorder API.
            Typically audio/webm;codecs=opus.

        mime_type : str
            MIME type of the audio.  Passed to the API so it knows the format.

        Returns
        -------
        str
            Transcribed text, stripped of leading/trailing whitespace.
            Returns an empty string if there was nothing to transcribe.

        Raises
        ------
        RuntimeError
            Wraps any Groq API error so callers don't need to import groq.
        """
        if not audio_bytes:
            return ""

        # Groq SDK needs a real file path — it reads the extension to validate format.
        # We use NamedTemporaryFile to get a safe unique path, write our bytes,
        # make the API call, then delete the file in the `finally` block.
        suffix = self._mime_to_extension(mime_type)
        tmp_path = None

        try:
            with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as tmp:
                tmp.write(audio_bytes)
                tmp_path = tmp.name  # remember path for cleanup

            with open(tmp_path, "rb") as audio_file:
                response = self._client.audio.transcriptions.create(
                    file=(f"audio{suffix}", audio_file, mime_type),
                    model=settings.groq_model,
                    response_format="text",   # returns a plain string, not JSON
                    language=settings.language or None,  # None = auto-detect
                    temperature=0.0,          # deterministic output
                )

            # The SDK returns a plain str when response_format="text"
            text = response if isinstance(response, str) else response.text
            return text.strip()

        except Exception as exc:
            raise RuntimeError(f"Groq transcription failed: {exc}") from exc

        finally:
            # Always delete the temp file, even if the API call raised
            if tmp_path and os.path.exists(tmp_path):
                os.unlink(tmp_path)

    @staticmethod
    def _mime_to_extension(mime_type: str) -> str:
        """
        Convert a MIME type string to a file extension.
        Falls back to .webm if the type is unrecognised.
        """
        mapping = {
            "audio/webm":        ".webm",
            "audio/webm;codecs=opus": ".webm",
            "audio/ogg":         ".ogg",
            "audio/wav":         ".wav",
            "audio/mp4":         ".mp4",
            "audio/mpeg":        ".mp3",
        }
        # Strip parameters like ";codecs=opus" for the lookup key
        base = mime_type.split(";")[0].strip().lower()
        return mapping.get(base, ".webm")
