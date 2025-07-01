#!/bin/bash -eux

# this script removes all configuration so that you could start over

rm -rf "${HOME}/.eclipse"
rm -rf "${HOME}/.stm32cubeide" "${HOME}/.stm32cubemx" "${HOME}/.stmcube" "${HOME}/.stmcufinder"
rm -rf "${HOME}/.streamlit"
