from django.contrib.auth import authenticate
from rest_framework.authentication import BasicAuthentication
from rest_framework.exceptions import AuthenticationFailed

# define an authentication to pass the tests
class CustomBasicAuthentication(BasicAuthentication):

    def authenticate_header(self, request):
        return 'Basic'

    def authenticate(self, request):
        try:
            return super().authenticate(request)
        except AuthenticationFailed:
            # Raise authentication failed exception
            raise AuthenticationFailed(self.get_custom_error_response())

    def get_custom_error_response(self):
        return {'message': 'Authentication Failed'}