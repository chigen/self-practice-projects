from django.contrib.auth.models import User
from django.http import Http404
from rest_framework import status
from rest_framework.response import Response
from rest_framework.views import APIView
from rest_framework.authentication import BasicAuthentication
from rest_framework.permissions import IsAuthenticated

from .serializers import UserSerializer
from .models import Profile
from .custom_authentication import CustomBasicAuthentication

class SignupView(APIView):
    # POST /signup
    def post(self, request, format=None):
        # print(request.data)
        serializer = UserSerializer(data=request.data)
        # print(f"initiated serializer: {serializer}")
        # print(f"serializer.is_valid(): {serializer.is_valid()}")
        if serializer.is_valid():
            # print("This signup request is valid")
            user_id = serializer.validated_data['username']
            if User.objects.filter(username=user_id).exists():
                return Response(
                    {"message": "Account creation failed", 
                     "cause": "already same user_id is used"},
                    status=status.HTTP_400_BAD_REQUEST
                )
            user = serializer.save()
            user_id = user.username
            profile_data = serializer.validated_data.get('profile', {})
            nickname = profile_data.get('nickname', user_id)
            return Response({
                "message": "Account successfully created",
                "user": {
                    "user_id": user_id,
                    "nickname": nickname,
                }
            }, status=status.HTTP_200_OK)
        # print(serializer.errors)
        return Response({
            "message": "Account creation failed",
            "cause": "required user_id and password",
        }, status=status.HTTP_400_BAD_REQUEST)
    
class UserDetailAndUpdateView(APIView):
    authentication_classes = [CustomBasicAuthentication]
    permission_classes = [IsAuthenticated]

    def get_object(self, user_id):
        try:
            return User.objects.get(username=user_id)
        except User.DoesNotExist:
            return None

    # GET /users/{user_id}
    def get(self, request, user_id, format=None):
        user = self.get_object(user_id)
        if user is None:
            return Response({"message": "No User found"}, 
                            status=status.HTTP_404_NOT_FOUND)
        # print(f"request.user is {request.user}")
        # print(f"user is {user}")

        serializer = UserSerializer(user)
        # print(f"serializer.data is {serializer.data}")
        get_output = {
            "user_id": serializer.data['user_id'],
            "nickname": serializer.data['nickname'],
        }
        if serializer.data['comment'] != '':
            get_output["comment"] = serializer.data['comment']

        return Response({
            "message": "User details by user_id",
            "user": get_output
        }, status=status.HTTP_200_OK)
    
    # PACTH /users/{user_id}
    def patch(self, request, user_id, format=None):
        user = self.get_object(user_id)
        # print(f"PACTH request.user is {request.user}")
        # print(f"PACTH data is {request.data}")
        if user is None:
            return Response({"message": "No User found"}, 
                            status=status.HTTP_404_NOT_FOUND)
        
        if user != request.user:
            return Response({"message": "No Permission for Update"}, 
                            status=status.HTTP_403_FORBIDDEN)
        if request.data == {}:
            return Response({"message": "User updation failed", 
                             "cause": "required nickname or comment"}, 
                             status=status.HTTP_400_BAD_REQUEST)
        
        serializer = UserSerializer(user, data=request.data, partial=True)
        if serializer.is_valid() and 'user_id' not in request.data and 'password' not in request.data:
            serializer.save()
            return Response({"message": "User successfully updated", 
                             "recipe": request.data}, 
                             status=status.HTTP_200_OK)
        
        return Response({"message": "User updation failed", 
                         "cause": "not updatable user_id and password"}, 
                         status=status.HTTP_400_BAD_REQUEST)

class CloseAccountView(APIView):
    authentication_classes = [CustomBasicAuthentication]
    permission_classes = [IsAuthenticated]
    # POST /close
    def post(self, request, format=None):
        request.user.delete()
        return Response({"message": "Account and user successfully removed"}, status=status.HTTP_200_OK)