from rest_framework.views import exception_handler
from rest_framework.exceptions import AuthenticationFailed
from rest_framework.exceptions import NotAuthenticated
from rest_framework.response import Response
from rest_framework import status

def custom_exception_handler(exc, context):
    # Call REST framework's default exception handler first
    response = exception_handler(exc, context)

    if isinstance(exc, AuthenticationFailed):
        custom_response_data = {'message': 'Authentication Failed'}
        return Response(custom_response_data, status=status.HTTP_401_UNAUTHORIZED)

    # if missing authentication credentials
    if isinstance(exc, NotAuthenticated):
        custom_response_data = {'message': 'Authentication Failed'}
        return Response(custom_response_data, status=status.HTTP_401_UNAUTHORIZED)
    return response
