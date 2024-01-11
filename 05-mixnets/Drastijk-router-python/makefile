PYTHON_VERSION := 3.11
UNAME ?= $(shell uname)
VENV ?= ./.venv
VENV_PYTHON ?= $(VENV)/bin/python
VENV_BIN ?= $(VENV)/bin/
CODE_DIRS = ./
export MYPYPATH = foo/


ifeq ($(UNAME),)
	UNAME := Windows
	PYTHON_VERSION :=
	VENV := .\.venv
	VENV_PYTHON := $(VENV)\Scripts\python.exe
	VENV_BIN := $(VENV)\Scripts\\
endif


init:
	python$(PYTHON_VERSION) -m venv $(VENV)
	$(VENV_PYTHON) -m pip install --upgrade pip
	$(VENV_PYTHON) -m pip install poetry
	$(VENV_PYTHON) -m poetry install --no-root

lint:
	$(VENV_BIN)flake8 $(CODE_DIRS)
	$(VENV_BIN)mypy --install-types --non-interactive $(CODE_DIRS)

pretty:
	$(VENV_BIN)isort $(CODE_DIRS)
	$(VENV_BIN)black $(CODE_DIRS)

plint: pretty lint
