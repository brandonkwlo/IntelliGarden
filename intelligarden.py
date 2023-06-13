import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore

if __name__ == "__main__":
    cred = credentials.Certificate("esp32-demo-4bf57-firebase-adminsdk-9akqz-a4733371dd.json")
    app = firebase_admin.initialize_app(cred)
    db = firestore.client()

    while True:
        user_cmd = input('> ')

        if user_cmd.lower().strip() == 'quit':
            break
        elif user_cmd.lower().strip() == 'read':
            readings = db.collection(u'Data').document('TEMPORARY').get().to_dict()
            
            for key in readings:
                print("{data}: {value}".format(data = key, value = readings[key]))
        elif user_cmd.lower().strip() == 'water':
            db.collection(u'Data').document('MANUAL').set({'water' : True})

            print("NOW WATERING...")
