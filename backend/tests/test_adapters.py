from datetime import timezone
import pytest
from market_data.adapters import utc_iso, AdapterError, validate_source_url

def test_unix_seconds_to_utc():
    assert utc_iso(0,"s") == "1970-01-01T00:00:00+00:00"

def test_iso_naive_assumed_utc():
    assert utc_iso("2025-01-02T03:04:05").endswith("+00:00")

def test_reject_http_source():
    with pytest.raises(AdapterError):
        validate_source_url("http://example.com/api")

def test_reject_localhost():
    with pytest.raises(AdapterError):
        validate_source_url("https://127.0.0.1/api")
