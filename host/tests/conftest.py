import pytest


def pytest_collection_modifyitems(config, items):
    """Tests on a real device switch ports: run them only on request (-m board)."""
    if "board" in (config.getoption("-m") or ""):
        return
    skip = pytest.mark.skip(reason="board tests run only with: pytest -m board")
    for item in items:
        if "board" in item.keywords:
            item.add_marker(skip)
